#include "stdafx.h"
#include "http_request.h"
#include "../external/curl/include/curl.h"
#include "../external/openssl/openssl/crypto.h"
#include "tools/system.h"
#include "log/log.h"
#include "utils.h"
#include "async_task.h"
#include "core.h"
#include "proxy_settings.h"
#include "network_log.h"

using namespace core;


class http_handles : public core::ithread_callback
{
    std::map<boost::thread::id, CURL*> handles_;

    std::mutex sync_mutex_;

public:

    http_handles()
    {
        
    }

    virtual ~http_handles()
    {
        assert(handles_.empty());
    }

    bool initialize()
    {
        return true;
    }

    CURL* get_for_this_thread()
    {
        boost::thread::id thread_id = boost::this_thread::get_id();

        std::lock_guard<std::mutex> lock(sync_mutex_);

        auto iter_handle = handles_.find(thread_id);
        if (iter_handle != handles_.end())
            return iter_handle->second;

        CURL* curl_handle = curl_easy_init();

        assert(curl_handle);

        handles_[thread_id] = curl_handle;

        return curl_handle;
    }

    virtual void on_thread_shutdown()
    {
        boost::thread::id thread_id = boost::this_thread::get_id();

        std::lock_guard<std::mutex> lock(sync_mutex_);

        auto iter_handle = handles_.find(thread_id);
        if (iter_handle != handles_.end())
        {
            curl_easy_cleanup(iter_handle->second);

            handles_.erase(iter_handle);
        }
    }
};

http_handles* g_handles = 0;


static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
static int32_t progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded);
static int32_t trace_function(CURL* _handle, curl_infotype _type, unsigned char* _data, size_t _size, void* _userp);

struct curl_context
{
    CURL* curl_;
    std::shared_ptr<tools::binary_stream> output_;
    std::function<bool()> is_stop_;

    curl_slist* header_chunk_;
    struct curl_httppost* post;
    struct curl_httppost* last;

    bool need_log_;
    bool keep_alive_;
        
    curl_context(std::function<bool()> _stop_func, bool _keep_alive)
        :	
        curl_(_keep_alive ? g_handles->get_for_this_thread() : curl_easy_init()),
        output_(new core::tools::binary_stream()),
        is_stop_(_stop_func),
        header_chunk_(0),
        post(NULL),
        last(NULL),
        need_log_(true),
        keep_alive_(_keep_alive)
    {
    }

    ~curl_context()
    {
        if (!keep_alive_)
            curl_easy_cleanup(curl_);

        if (header_chunk_)
            curl_slist_free_all(header_chunk_);
    }

    bool init(int32_t _connect_timeout, int32_t _timeout, core::proxy_settings _proxy_settings)
    {
        if (!curl_)
            return false;

        if (keep_alive_)
            curl_easy_reset(curl_);

        if (platform::is_windows() && !platform::is_windows_vista_or_late())
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        else
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);

        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);

        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*) this);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_memory_callback);
        curl_easy_setopt(curl_, CURLOPT_PROGRESSDATA, (void*) this);
        curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl_, CURLOPT_PROGRESSFUNCTION, progress_callback);

        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 5L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 5L);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

        std::string user_agent = core::utils::get_user_agent();
        curl_easy_setopt(curl_, CURLOPT_USERAGENT, user_agent.c_str());

        
        curl_easy_setopt(curl_, CURLOPT_DEBUGDATA, (void*) this);
        curl_easy_setopt(curl_, CURLOPT_DEBUGFUNCTION, trace_function);
        curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);

        if (_timeout > 0)
            curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, _timeout);

        if (_connect_timeout > 0)
            curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, _connect_timeout);

        if (_proxy_settings.use_proxy_)
        {
            curl_easy_setopt(curl_, CURLOPT_PROXY, tools::from_utf16(_proxy_settings.proxy_server_).c_str());
            curl_easy_setopt(curl_, CURLOPT_PROXYTYPE, _proxy_settings.proxy_type_);

            if (_proxy_settings.need_auth_)
            {
                curl_easy_setopt(curl_, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
                curl_easy_setopt(curl_, CURLOPT_PROXYUSERPWD, tools::from_utf16(_proxy_settings.login_ + L":" + _proxy_settings.password_).c_str());
            }
        }

        return true;
    }

    bool is_need_log()
    {
        return need_log_;
    }

    void set_need_log(bool _need)
    {
        need_log_ = _need;
    }

    void set_range(int64_t _from, int64_t _to)
    {
        assert(_from >= 0);
        assert(_to > 0);
        assert(_from < _to);

        std::stringstream ss_range;
        ss_range << _from << "-" << _to;

        curl_easy_setopt(curl_, CURLOPT_RANGE, ss_range.str().c_str());
    }

    void set_url(const char* sz_url)
    {
        curl_easy_setopt(curl_, CURLOPT_URL, sz_url);
    }

    void set_post()
    {
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    }

    void set_http_post()
    {
        curl_easy_setopt(curl_, CURLOPT_HTTPPOST, post);
    }

    void set_post_fields(const char* sz_fields, int32_t _size)
    {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, _size);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, sz_fields);
    }

    long get_response_code()
    {
        long response_code = 0;

        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);

        return response_code;
    }

    void set_modified_time(time_t _last_modified_time)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
        curl_easy_setopt(curl_, CURLOPT_TIMEVALUE, _last_modified_time);
    }

    std::shared_ptr<tools::binary_stream> get_response()
    {
        return output_;
    }

    void set_form_field(const char* _field_name, const char* _value)
    {
        curl_formadd(&post, &last, CURLFORM_COPYNAME, _field_name,
            CURLFORM_COPYCONTENTS, _value, CURLFORM_END);
    }

    void set_form_file(const char* _field_name, const char* _file_name)
    {
        curl_formadd(&post, &last, CURLFORM_COPYNAME, _field_name, CURLFORM_FILE, _file_name,
            CURLFORM_END);
    }

    void set_form_filedata(const char* _field_name, const char* _file_name, tools::binary_stream& _data)
    {
        const auto size = _data.available();
        const auto data = _data.read_available();
        curl_formadd(&post, &last,
                     CURLFORM_COPYNAME, _field_name,
                     CURLFORM_BUFFER, _file_name,
                     CURLFORM_BUFFERPTR, data,
                     CURLFORM_BUFFERLENGTH, size,
                     CURLFORM_CONTENTTYPE, "application/octet-stream",
                     CURLFORM_END);
    }
    
    bool execute_request()
    {
        CURLcode res = curl_easy_perform(curl_);

        return (res == CURLE_OK);
    }

    void set_custom_header_params(const std::list<std::string>& _params)
    {
        for (auto parameter : _params)
            header_chunk_ = curl_slist_append(header_chunk_, parameter.c_str());

        if (header_chunk_)
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_chunk_);
    }
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;

    ((curl_context*) userp)->output_->write((char*) contents, (uint32_t)realsize);

    return realsize;
}

static int32_t progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
    if (((curl_context*) ptr)->is_stop_())
        return -1;

    return 0;
}

static void dump_function(const char *text, std::stringstream& _ss_out, unsigned char *ptr, size_t size, bool nohex)
{
//   size_t i;
//    size_t c;

//    unsigned int width=0x10;

//    if(nohex)
        /* without the hex output, we can fit more on screen */ 
//            width = 0x80;

    char buffer[1024] = {0};

    sprintf(buffer, "%s, %10.10ld bytes (0x%8.8lx)\n",
        text, (long)size, (long)size);

    _ss_out << buffer;

    for (size_t i = 0; i < size; i++)
    {
    //    sprintf(buffer, "%4.4lx: ", (long)i);
    //    _ss_out << buffer;

    /*    if(!nohex) {
            
            for(c = 0; c < width; c++)
                if(i+c < size)
                {
                    sprintf(buffer, "%02x ", ptr[i+c]);
                    _ss_out << buffer;
                }
                else
                {
                    _ss_out << "   ";
                }
        }*/

      /*  for(c = 0; (c < width) && (i+c < size); c++) {
            // check for 0D0A; if found, skip past and start a new line of output 
            if (nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
                i+=(c+2-width);
                break;
            }*/

            sprintf(buffer, "%c", (ptr[i]>=0x20) && (ptr[i]<0x80) ? ptr[i] : '.');
            _ss_out << buffer;

        /*    // check again for 0D0A, to avoid an extra \n if it's at width 
            if (nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
                i+=(c+3-width);
                break;
            }
        }*/
    }
}

std::vector<std::string> get_filter_keywords()
{
    std::vector<std::string> keywords;
    keywords.push_back("schannel:");
    keywords.push_back("STATE:");

    return keywords;
}

const std::string filter1 = "schannel:";
const std::string filter2 = "STATE:";


static int32_t trace_function(CURL* _handle, curl_infotype _type, unsigned char* _data, size_t _size, void* _userp)
{
    if (!((curl_context*) _userp)->is_need_log())
        return 0;
            
    std::stringstream ss;

    const char *text = "";

    (void)_userp;
    (void)_handle; /* prevent compiler warning */ 

    switch (_type) 
    {
    case CURLINFO_TEXT:
        {
            if (filter1.length() < _size && memcmp(_data, filter1.c_str(), filter1.length()) == 0)
                return 0;

            if (filter2.length() < _size && memcmp(_data, filter2.c_str(), filter2.length()) == 0)
                return 0;
        }
        break;
    case CURLINFO_HEADER_OUT:
    //    text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
    case CURLINFO_SSL_DATA_OUT:
    //    text = "=> Send data";
        break;
    case CURLINFO_HEADER_IN:
    //    text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
    case CURLINFO_SSL_DATA_IN:
    //    text = "<= Recv data";
        break;
    default:
        break;
    }

    tools::binary_stream bs;

    if (*text != '\0')
    {
        bs.write<std::string>(text);
        bs.write<std::string>("\n");
    }

    bs.write((const char*)_data, (uint32_t)_size);

    g_core->get_network_log().write_data(bs);

    bs.reset();

    return 0;	
}


http_request_simple::http_request_simple(std::function<bool()> stop_func)
    :	is_stop_(stop_func),
    output_(new tools::binary_stream()),
    is_time_condition_(false),
    last_modified_time_(0),
    post_data_(0),
    post_data_size_(0),
    copy_post_data_(false),
    timeout_(-1),
    connect_timeout_(-1),
    range_from_(-1),
    range_to_(-1),
    is_post_form_(false),
    need_log_(true),
    keep_alive_(false)
{
}

http_request_simple::~http_request_simple()
{
    clear_post_data();
}

void http_request_simple::set_need_log(bool _need)
{
    need_log_ = _need;
}

void http_request_simple::push_post_parameter(const std::wstring& name, const std::wstring& value)
{
    assert(!name.empty());

    post_parameters_[tools::from_utf16(name)] = tools::from_utf16(value);
}

void http_request_simple::push_post_parameter(const std::string& name, const std::string& value)
{
    post_parameters_[name] = value;
}

void http_request_simple::push_post_form_parameter(const std::wstring& name, const std::wstring& value)
{
    post_form_parameters_[tools::from_utf16(name)] = tools::from_utf16(value);
}

void http_request_simple::push_post_form_parameter(const std::string& name, const std::string& value)
{
    assert(!name.empty());
    post_form_parameters_[name] = value;
}

void http_request_simple::push_post_form_file(const std::wstring& name, const std::wstring& file_name)
{
    push_post_form_file(tools::from_utf16(name), tools::from_utf16(file_name));
}

void http_request_simple::push_post_form_file(const std::string& name, const std::string& file_name)
{
    assert(!name.empty());
    assert(!file_name.empty());
    post_form_files_.insert(std::make_pair(name, file_name));
}

void http_request_simple::push_post_form_filedata(const std::wstring& name, const std::wstring& file_name)
{
    assert(!name.empty());
    assert(!file_name.empty());
    file_binary_stream file_data;
    file_data.file_name_ = file_name.substr(file_name.find_last_of(L"\\/") + 1);
    file_data.file_stream_.load_from_file(file_name);
    if (file_data.file_stream_.available())
        post_form_filedatas_.insert(std::make_pair(tools::from_utf16(name), file_data));
}

void http_request_simple::set_url(const std::wstring& url)
{
    set_url(tools::from_utf16(url));
}

void http_request_simple::set_url(const std::string& url)
{
    url_ = url;
}

void http_request_simple::set_modified_time_condition(time_t _modified_time)
{
    is_time_condition_ = true;
    last_modified_time_ = _modified_time;
}

void http_request_simple::set_connect_timeout(int32_t _timeout_ms)
{
    connect_timeout_ = _timeout_ms;
}

void http_request_simple::set_timeout(int32_t _timeout_ms)
{
    timeout_ = _timeout_ms;
}

std::string http_request_simple::get_post_param() const
{
    std::string result = ""; 
    if (!post_parameters_.empty())
    {
        std::stringstream ss_post_params;

        for (auto iter = post_parameters_.begin(); iter != post_parameters_.end(); ++iter)
        {
            if (iter != post_parameters_.begin())
                ss_post_params << '&';

            ss_post_params << iter->first;

            if (!iter->second.empty())
                ss_post_params << '=' << iter->second;
        }

        result = ss_post_params.str();
    }
    return result;
}

void http_request_simple::set_post_params(curl_context* _ctx)
{
    auto post_params = get_post_param();

    if (!post_params.empty())
        set_post_data(post_params.c_str(), (int32_t)post_params.length(), true);

    for (auto iter = post_form_parameters_.begin(); iter != post_form_parameters_.end(); ++iter)
    {
        if (!iter->second.empty())
            _ctx->set_form_field(iter->first.c_str(), iter->second.c_str());
    }

    for (auto iter = post_form_files_.begin(); iter != post_form_files_.end(); ++iter)
    {
        _ctx->set_form_file(iter->first.c_str(), iter->second.c_str());
    }

    for (auto iter = post_form_filedatas_.begin(); iter != post_form_filedatas_.end(); ++iter)
    {
        _ctx->set_form_filedata(iter->first.c_str(), tools::from_utf16(iter->second.file_name_).c_str(), iter->second.file_stream_);
    }

    if (post_data_ && post_data_size_)
        _ctx->set_post_fields(post_data_, post_data_size_);

    if (is_post_form_)
        _ctx->set_http_post();
    else
        _ctx->set_post();
}

bool http_request_simple::send_request(bool _post, bool switch_proxy)
{
    curl_context ctx(is_stop_, keep_alive_);

    if (!ctx.init(connect_timeout_, timeout_, switch_proxy ? g_core->get_registry_proxy_settings() : g_core->get_proxy_settings()))
    {
        assert(false);
        return false;
    }

    if (_post)
        set_post_params(&ctx);

    if (is_time_condition_)
        ctx.set_modified_time(last_modified_time_);

    if (range_from_ >= 0 && range_to_ > 0)
        ctx.set_range(range_from_, range_to_);

    ctx.set_need_log(need_log_);

    ctx.set_custom_header_params(custom_headers_);
    ctx.set_url(url_.c_str());

    static std::atomic<bool> first(true);
    
    if (!ctx.execute_request())
    {
        if (first)
        {
            if (switch_proxy)
                return false;
            
            return send_request(_post, true);
        }
        return false;
    }

    first = false;
    if (switch_proxy)
        g_core->switch_proxy_settings();

    response_code_ = ctx.get_response_code();
    output_ = ctx.get_response();

    return true;
}

bool http_request_simple::post()
{
    return send_request(true);
}

bool http_request_simple::get()
{
    return send_request(false);
}

void http_request_simple::set_range(int64_t _from, int64_t _to)
{
    range_from_ = _from;
    range_to_ = _to;
}

std::shared_ptr<tools::binary_stream> http_request_simple::get_response()
{
    return output_;
}

long http_request_simple::get_response_code()
{
    return response_code_;
}

void http_request_simple::get_post_parameters(std::map<std::string, std::string>& _params)
{
    _params = post_parameters_;
}

void http_request_simple::set_custom_header_param(const std::string& _value)
{
    custom_headers_.push_back(_value);
}

void http_request_simple::clear_post_data()
{
    if (copy_post_data_ && post_data_)
        free(post_data_);

    copy_post_data_ = false;
    post_data_ = 0;
    post_data_size_ = 0;
}

void http_request_simple::set_post_data(const char* _data, int32_t _size, bool _copy_post_data)
{
    assert(_data);
    assert(_size);


    clear_post_data();

    copy_post_data_ = _copy_post_data;
    post_data_size_ = _size;

    if (_copy_post_data)
    {
        post_data_ = (char*) malloc(_size);
        memcpy(post_data_, _data, _size);
    }
    else
    {
        post_data_ = (char*) _data;
    }
}


std::vector<std::mutex*> http_request_simple::ssl_sync_objects;

static unsigned long id_function(void)
{
    return tools::system::get_current_thread_id();
}


static void locking_function( int mode, int n, const char *file, int line )
{
    if ( mode & CRYPTO_LOCK )
        http_request_simple::ssl_sync_objects[n]->lock();
    else
        http_request_simple::ssl_sync_objects[n]->unlock();
}


void core::http_request_simple::init_global()
{
    auto lock_count = CRYPTO_num_locks();
    http_request_simple::ssl_sync_objects.resize(lock_count);
    for (int i = 0;  i < lock_count;  i++)
        http_request_simple::ssl_sync_objects[i] = new std::mutex();

    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);

#ifdef _WIN32
    curl_global_init(CURL_GLOBAL_ALL);
#else
    curl_global_init(CURL_GLOBAL_SSL);
#endif
}

void core::http_request_simple::shutdown_global()
{
    if (!http_request_simple::ssl_sync_objects.size())
        return;

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    for (int i = 0;  i < CRYPTO_num_locks(  );  i++)
        delete(http_request_simple::ssl_sync_objects[i]);

    http_request_simple::ssl_sync_objects.clear();
}

void core::http_request_simple::set_post_form(bool _is_post_form)
{
    is_post_form_ = _is_post_form;
}

void core::http_request_simple::set_keep_alive()
{
    if (keep_alive_)
        return;
    
    keep_alive_ = true;

    custom_headers_.push_back("Connection: keep-alive");
}

ithread_callback* core::http_request_simple::create_http_handlers()
{
    assert(!g_handles);
    g_handles = new http_handles();
    return g_handles;
}

std::string core::http_request_simple::get_post_url()
{
    return url_ + "?" + get_post_param();
}
