#ifdef _WIN32
#pragma once

namespace core
{
    namespace dump
    {
        // NOTE : this values must coincide with https://rink.hockeyapp.net
        const std::string product_bundle = "icq.dekstop";

        static std::wstring product_data_path;
        void set_product_data_path(const std::wstring& _product_data_path);
        std::wstring get_report_path();

        static std::string os_version = "Windows";
        void set_os_version(const std::string& _os_version);
        std::string get_os_version();

        class crash_handler  
        {
        public:

            // Constructor
            crash_handler();

            // Destructor
            virtual ~crash_handler();

            // Sets exception handlers that work on per-process basis
            void set_process_exception_handlers();

            // Installs C++ exception handlers that function on per-thread basis
            void set_thread_exception_handlers();

            // Collects current process state.
            static void get_exception_pointers(
                DWORD dwExceptionCode, 
                EXCEPTION_POINTERS** pExceptionPointers);

            // This method creates minidump of the process
            static void create_mini_dump(EXCEPTION_POINTERS* pExcPtrs);

            static void create_log_file_for_hockey_app(EXCEPTION_POINTERS* pExcPtrs);

            /* Exception handler functions. */

            static LONG WINAPI seh_handler(PEXCEPTION_POINTERS pExceptionPtrs);
            static void __cdecl terminate_handler();
            static void __cdecl unexpected_handler();

            static void __cdecl pure_call_handler();

            static void __cdecl invalid_parameter_handler(const wchar_t* expression, 
                const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);

            static int __cdecl new_handler(size_t);

            static void sigabrt_handler(int);
            static void sigfpe_handler(int /*code*/, int subcode);
            static void sigint_handler(int);
            static void sigill_handler(int);
            static void sigsegv_handler(int);
            static void sigterm_handler(int);

        private:
            static bool need_write_dump();
            static int get_dump_type();
        };
    }
}

#endif // _WIN32
