#pragma once

#include "core/apps/app_base.h"

class SettingsApp : public AppBase {
public:
    SettingsApp(const AppInfo& info);
    virtual ~SettingsApp();
    
    virtual bool initialize() override;
    virtual bool start() override;
    virtual bool pauseApp() override;
    virtual bool resumeApp() override;
    virtual bool stop() override;
    virtual void cleanup() override;
    
    static AppInfo getAppInfo();
    
private:
    // Settings-specific members
};

class SettingsAppFactory : public AppFactory {
public:
    AppBase* createApp() override {
        return new SettingsApp(getAppInfo());
    }
    
    void destroyApp(AppBase* app) override {
        delete app;
    }
    
    AppBase::AppInfo getAppInfo() override {
        return SettingsApp::getAppInfo();
    }
};