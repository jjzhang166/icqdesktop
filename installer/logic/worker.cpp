#include "stdafx.h"
#include "worker.h"

#include "tools.h"

#include "tasks/copy_files.h"
#include "tasks/start_process.h"
#include "tasks/terminate_process.h"
#include "tasks/write_registry.h"
#include "tasks/create_links.h"
#include "tasks/update_from_8x_version.h"
#include "exported_data.h"


namespace installer
{
    namespace logic
    {
        worker current_worker;

        worker* get_worker()
        {
            return &current_worker;
        }

        worker::worker()
            :	progress_(0)
        {
        }

        worker::~worker()
        {
        }

        task_callback dummy_callback = [](const installer::error&){};

        void worker::reset_watcher()
        {
            current_task_watcher_.reset(new QFutureWatcher<installer::error>());
        }

        void worker::final_install()
        {
            run_async_function(store_exported_settings, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                }

                run_async_function(store_exported_account, [this](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                    }

                    run_async_function(start_process, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        emit finish();

                    }, 100);

                }, 97);
            }, 93);
        }

        void worker::install()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function(export_from_8x_and_uninstall, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function(terminate_process, [this](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }

                    run_async_function(copy_files, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        run_async_function(write_registry, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function(create_links, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                if (get_exported_data().get_accounts().size())
                                {
                                    if (get_exported_data().get_accounts().size() > 1)
                                    {
                                        emit select_account();
                                        return;
                                    }
                                    else
                                    {
                                        get_exported_data().set_exported_account(*get_exported_data().get_accounts().begin());
                                    }
                                }

                                final_install();

                            }, 85);
                        }, 60);
                    }, 40);
                }, 30);
            }, 20 );
        }

        void worker::uninstall()
        {
            progress_ = 10;
            emit progress(progress_);

            run_async_function(terminate_process, [this](const installer::error&)
            {
                run_async_function(clear_registry, [this](const installer::error&)
                {
                    run_async_function(delete_links, [this](const installer::error&)
                    {
                        run_async_function(delete_files, [this](const installer::error&)
                        {
                            run_async_function(delete_self, [this](const installer::error&)
                            {
                                emit finish();
                            }, 100);
                        }, 80);
                    }, 60);
                }, 40);
            }, 20);
        }

        void worker::uninstalltmp()
        {
            QDir dir_product(get_product_folder());

            for (int i = 0; i < 50; i++)
            {
                if (dir_product.removeRecursively())
                    return;

                ::Sleep(100);
            }
        }

        void worker::clear_updates()
        {
            QDir dir_updates(get_updates_folder());

            for (int i = 0; i < 50; i++)
            {
                if (dir_updates.removeRecursively())
                    return;

                ::Sleep(100);
            }
        }

        void worker::update()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function(copy_files_to_updates, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function(write_update_version, [this](const installer::error&)
                {

                    emit finish();
                }, 100);

            }, 95);
        }

        void worker::update_final(CHandle& _mutex)
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function(copy_files_from_updates, [this, &_mutex](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function(write_to_uninstall_key, [this, &_mutex](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }

                    _mutex.Close();

                    run_async_function(start_process, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        run_async_function(copy_self_from_updates, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function(delete_updates, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                emit finish();

                            }, 100);
                        }, 90);
                    }, 80);
                }, 75);
            }, 70);
        }

        void worker::set_installed_flag_8x()
        {
            set_8x_update_downloaded();
        }

        void worker::copy_self_to_bin_8x()
        {
            copy_self_to_icqim_8x();
        }

        void worker::autoupdate_from_8x()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function(export_from_8x_and_uninstall, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function(terminate_process, [this](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }

                    run_async_function(copy_files, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        run_async_function(write_registry, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function(create_links, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                if (get_exported_data().get_accounts().size())
                                {
                                    if (get_exported_data().get_accounts().size() > 1)
                                    {
                                        emit select_account();
                                        return;
                                    }
                                    else
                                    {
                                        get_exported_data().set_exported_account(*get_exported_data().get_accounts().begin());
                                    }
                                }

                                final_install();

                            }, 85);
                        }, 60);
                    }, 40);
                }, 30);
            }, 20 );
        }
    }
}