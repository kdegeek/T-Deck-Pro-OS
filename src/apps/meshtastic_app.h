#pragma once

#include "core/apps/app_base.h"

class MeshtasticApp : public AppBase {
public:
    MeshtasticApp(const AppInfo& info);
    virtual ~MeshtasticApp();
    
    virtual bool initialize() override;
    virtual bool start() override;
    virtual bool pauseApp() override;
    virtual bool resumeApp() override;
    virtual bool stop() override;
    virtual void cleanup() override;
    
    static AppInfo getAppInfo();
    
private:
    // Meshtastic-specific members
};

class MeshtasticAppFactory : public AppFactory {
public:
    AppBase* createApp() override {
        return new MeshtasticApp(getAppInfo());
    }
    
    void destroyApp(AppBase* app) override {
        delete app;
    }
    
    AppBase::AppInfo getAppInfo() override {
        return MeshtasticApp::getAppInfo();
    }
};