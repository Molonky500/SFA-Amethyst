//not part of savedata because only used on Wii,
//where we can use a separate config file.
typedef enum {
    WII_SHAKE_TO_SWING = (1 <<  0), //shake remote to swing staff
    WII_SHAKE_TO_ROLL  = (1 <<  1), //shake nunchuk to roll
    WII_NUNCHUK_CAMERA = (1 <<  2), //tilt nunchuk to move camera
    WII_NUNCHUK_STEER  = (1 <<  3), //tilt nunchuk to steer mounts
} WiiOptionFlag;

//for orient:
//X is turn left/right (yaw) but Nunchuk doesn't provide it so it's always zero
//Y is tilt forward/back (pitch)
//Z is tilt left/right (roll)
typedef struct {
    vec3s deadMin; //min values for each axis deadzone
    vec3s deadMax; //max values for each axis deadzone
    s16 pitchMin, pitchMax; //valid range of pitch
    s16 rollMin, rollMax; //valid range of roll
} WiiCalibrationData;

typedef struct {
    u32 options; //WiiOptionFlag
    WiiCalibrationData calNunchuk;
} WiimoteConfig;

extern WiimoteConfig wiimoteCfg[GAME_MAX_WIIMOTES];
