typedef enum { //type:u32
	ObjSendMsgFlags_DontSendToSelf = 0x1,
	ObjSendMsgFlags_SendToAnyObj   = 0x2,
    ObjSendMsgFlags_UseDefNo       = 0x4,
} ObjSendMsgFlags;

typedef enum { //type:u32
	ObjMsg_CollectiblePickup = 0x0007000B,
    //sent from carryable every frame to player while carrying
    ObjMsg_CarryBarrel       = 0x00100008,
    ObjMsg_CarryBasket       = 0x00100010,
} ObjMsgId;

typedef struct PACKED ObjSeqMsg {
	ObjMsgId msg;
	ObjInstance* from;
	void* param;
} ObjMsg;

typedef struct PACKED ObjMsgQueue {
	int count; //current number of messages
	int size; //max number of messages
    ObjMsg msg[0];
} ObjMsgQueue;
