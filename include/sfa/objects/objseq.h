typedef enum { //type:u32
	ObjSendMsgFlags_DontSendToSelf = 0x1,
	ObjSendMsgFlags_SendToAnyObj   = 0x2,
    ObjSendMsgFlags_UseDefNo       = 0x4,
} ObjSendMsgFlags;

typedef enum { //type:u32
	ObjMsg_CollectiblePickup = 0x7000B,
} ObjMsg;

typedef struct PACKED ObjSeqMsg {
	ObjMsg msg;
	ObjInstance* from;
	void* param;
} ObjSeqMsg;

typedef struct PACKED ObjSeqMsgQueue {
	int nMsgs;
	int length;
    ObjSeqMsg msg[0]; //XXX verify
} ObjSeqMsgQueue;

typedef struct PACKED ObjSeqCmd {
	byte flags;
	u8   cmd;    //ObjSeqCmdEnum
	byte param1;
	byte param2;
} ObjSeqCmd;

typedef enum { //type: u8
	ParticleEffect = 0x3,
	Dummy04 = 0x4,
	LoadDll = 0x5,
	ScreenTransition = 0x6,
	SetGameBit = 0xb,
	ClearGameBit = 0xc,
	SetInputOverride = 0xd,
} ObjSeqCmdEnum2;

typedef struct ObjSeqCmd2 {
	ObjInstance *obj;
	s16 param;
	u8 cmd; //ObjSeqCmdEnum2
	s8 unk;
} ObjSeqCmd2;

typedef struct SeqBgCmdParam {
	s16 seqNo;
	s16 seqNo2;
	s16 nCmds;
} SeqBgCmdParam;

typedef enum { //type:u16
	JUMPTOTIME = 0x1,
	COUNTER = 0x2, //multiple subcommands
	COUNTER_ADD = 0x3,
	PAUSE = 0x4, //pause until condition `index` is false
	CONTINUE = 0x5, //end script
	SubCmd6 = 0x6,
	MESSAGE = 0x7,
	DECISION = 0x8, //set event type `index` to jump to label `param`
	JUMPTARGET = 0x9, //define label `param`
	JUMPTOLABEL = 0xa, //evaluate condition `index`; if true, goto label `param`
} ObjSeqCondOp;

typedef struct PACKED ObjSeqCondition {
	s16 param;
	//int index:10; //official name
	//ObjSeqCondOp op:6;
	u16 indexAndOp;
} ObjSeqCondition;

typedef struct {
	ObjSeqCondition *actions;
	s16 nCmds;
	s16 nextTime;
} ObjSeqSubCmdStruct;

typedef enum { //type:u32
	ObjSeqFnReturn_Stop = 0x4,
	ObjSeqFnReturn_Continue = 0x0,
} ObjSeqFnReturn;

typedef ObjSeqFnReturn (*ObjSeqFn)(
    ObjInstance* param1,
    ObjInstance* param2,
    void* param3 //ObjState_AnimatedObj*
);
