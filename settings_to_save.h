struct SettingsToSave {
    int windowX;
    int windowY;
    int windowPosX;
    int windowPosY;
    int themeIndex;
    int useRadians;
    int startUseRadians;  //NOTE: This is to keep it deterministic, since we run all the code each time, the useRadians is really just a visual to the user but doesn't actually control the value
    char *code;
};