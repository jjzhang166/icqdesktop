#include "stdafx.h"
#include "worker.h"

#include "tools.h"

#include "tasks/copy_files.h"
#include "tasks/start_process.h"
#include "tasks/terminate_process.h"
#include "tasks/write_registry.h"
#include "tasks/create_links.h"
#include "tasks/update_from_8x_version.h"
#include "tasks/unistall_8x.h"
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
            :   progress_(0),
                delete_8x_files_on_final_(false),
                delete_8x_self_on_final_(false)
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
            run_async_function(store_exported_settings, [this](const installer::error& /*_err*/)
            {

                run_async_function(store_exported_account, [this](const installer::error& /*_err*/)
                {

                    run_async_function(start_process, [this](const installer::error& /*_err*/)
                    {

                        if (is_delete_8x_files_on_final())
                        {
                            delete_8x_registry_and_files();
                        }

                        if (is_delete_8x_self_on_final())
                        {
                            run_async_function(delete_self_from_8x, [this](const installer::error& /*_err*/)
                            {

                                emit finish();
                            }, 100);
                        }
                        else
                        {
                            emit finish();
                        }

                    }, 100);

                }, 97);
            }, 93);
        }

        void worker::install()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function(export_from_8x, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }
                run_async_function(uninstall_8x_from_executable, [this](const installer::error& _err)
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
                }, 20);
            }, 10 );
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
                            run_async_function(delete_self_and_product_folder, [this](const installer::error&)
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
            QString folder_for_delete = get_product_folder();

            if (!get_install_config().get_folder_for_delete().isEmpty())
            {
                folder_for_delete = get_install_config().get_folder_for_delete();
            }

            QDir delete_dir(folder_for_delete);

            for (int i = 0; i < 50; i++)
            {
                if (delete_dir.removeRecursively())
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

        void worker::update_from_8x_step_1()
        {
            run_async_function(copy_files, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);

                    return;
                }

                run_async_function(set_8x_update_downloaded, [this](const installer::error& /*_err*/)
                {
                    copy_self_to_bin_8x();

                    emit finish();
                }, 100);

            }, 50);
        }

        void worker::update_from_8x_step_2()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function(export_from_8x, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function(delete_8x_links, [this](const installer::error& /*_err*/)
                {
                    run_async_function(terminate_process, [this](const installer::error& _err)
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

                                set_delete_8x_files_on_final(true);
                                set_delete_8x_self_on_final(true);

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
                    }, 30);
                }, 15);

            }, 10 );
        }

        void worker::copy_self_to_bin_8x()
        {
            copy_self_to_icqim_8x();
        }
    }
}