typedef enum {
    CAM_FLAG_PAD3         = (1 << 0), //use controller 3 to move
    CAM_FLAG_INVERT_X     = (1 << 1), //invert X axis movement
    CAM_FLAG_INVERT_Y     = (1 << 2), //invert Y axis movement
    CAM_FLAG_NO_HUD       = (1 << 3), //disable the HUD
    CAM_FLAG_NO_LETTERBOX = (1 << 4), //disable letterboxing in cutscenes
    CAM_FLAG_PLAYER_AXIS  = (1 << 5), //rotate around player, not own axis
} CameraFlags; //u8

typedef enum {
    CAM_MODE_NORMAL = 0,
    CAM_MODE_STAY,
    CAM_MODE_FREE,
    CAM_MODE_ORBIT,
    CAM_MODE_BIRD,
    CAM_MODE_FIRST_PERSON,
    NUM_CAM_MODES,
} DebugCameraMode;

typedef enum {
    CAM_DLL_NORMAL = 0x42, //42 general gameplay
    CAM_DLL_STAFF_ANIM, //43 related to camera, staff booster, lifting rocks; activated for 1 frame by half-pressing L
    CAM_DLL_VIEWFINDER, //44 zoom goggles
    CAM_DLL_BIKE, //45 riding bike
    CAM_DLL_DEBUG, //46
    CAM_DLL_TEST_STRENGTH, //47 Test of Strength game
    CAM_DLL_STATIC, //48 static camera, with icon on HUD
    CAM_DLL_COMBAT, //49 focused on enemy
    CAM_DLL_SHIP_BATTLE, //4A opening scene
    CAM_DLL_CLIMB, //4B climbing
    CAM_DLL_FIXED, //4C cutscene control
    CAM_DLL_NPC_SPEAK, //4D speaking to NPC or feeding Tricky
    CAM_DLL_WORLD_MAP, //4E
    CAM_DLL_UNK4F, //4F
    CAM_DLL_CRAWL, //50 crawling through tunnels
    CAM_DLL_CANNON, //51 aiming a cannon
    CAM_DLL_FORCE_BEHIND, //52 holding L, aiming, etc
    CAM_DLL_CLOUD_RUNNER, //53 riding in Dragon Rock, others?
    CAM_DLL_DRAKOR, //54
    CAM_DLL_UNK55, //55 view from below
    CAM_DLL_ARWING, //56 Arwing levels
    CAM_DLL_TITLE //57 title screen
} CameraDll; //also camera mode

//turns out gamecube had joycon drift before joycons did
#define CAMERA_LSTICK_DEADZONE 4
#define CAMERA_RSTICK_DEADZONE 4
#define CAMERA_TRIGGER_DEADZONE 4

#define S16_TO_RADIANS (1.0 / 10430.37835)

//camera.c
extern u8 cameraFlags; //CameraFlags
extern s8 debugCameraMode; //DebugCameraMode
void cameraUpdateHook();
u32 minimapButtonHeldHook(int pad);
u32 minimapButtonPressedHook(int pad);
int viewFinderZoomHook(int pad);
