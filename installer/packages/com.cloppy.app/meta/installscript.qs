function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut",
                               "@TargetDir@/Cloppy.exe",
                               "@StartMenuDir@/Cloppy.lnk",
                               "workingDirectory=@TargetDir@",
                               "iconPath=@TargetDir@/Cloppy.exe",
                               "description=Cloppy 클립보드 매니저");

        if (installer.value("createDesktopShortcut", "true") === "true") {
            component.addOperation("CreateShortcut",
                                   "@TargetDir@/Cloppy.exe",
                                   "@DesktopDir@/Cloppy.lnk",
                                   "workingDirectory=@TargetDir@",
                                   "iconPath=@TargetDir@/Cloppy.exe",
                                   "description=Cloppy 클립보드 매니저");
        }
    }
}
