#pragma once

#include "core/apps/app_base.h"

class FileManagerApp : public AppBase {
public:
    FileManagerApp(const AppInfo& info);
    virtual ~FileManagerApp();
    
    virtual bool initialize() override;
    virtual bool start() override;
    virtual bool pauseApp() override;
    virtual bool resumeApp() override;
    virtual bool stop() override;
    virtual void cleanup() override;
    
    static AppInfo getAppInfo();
    
private:
    // File manager-specific members
};

class FileManagerAppFactory : public AppFactory {
public:
    AppBase* createApp() override {
        return new FileManagerApp(getAppInfo());
    }
    
    void destroyApp(AppBase* app) override {
        delete app;
    }
    
    AppBase::AppInfo getAppInfo() override {
        return FileManagerApp::getAppInfo();
    }
};