#include "main.h"
#include "revolution/os.h"

void drawHitbox(ObjInstance *obj) {
    Model *model = objGetCurModelPtr(obj);
    Color4b color;

    /* XXX this renders "ghosts" - hitboxes for objects that are actually
    somewhere else. It seems the hitboxes actually are moving when you
    move between map cells. Something else prevents them from being used,
    but I can't find what. */

    HitState *hits = obj->hitstate;
    //if(!hits || hits->unk70) return;
    for(int iSphere=0; iSphere < model->header->nHitSpheres; iSphere++) {
        HitSpherePos *pos = &model->curHitSpherePos[iSphere];
        HitSphere *sphere = &model->header->hitSpheres[iSphere];
        color.value = 0x7F;
        if(hits && !hits->disable) {
            u8 s=128, v=128;
            for(int iHit=0; iHit < hits->nHits; iHit++) {
                if(hits->sphereIdxs[iHit] == iSphere) {
                    //XXX not very useful.
                    //maybe we can just map the hit types (there aren't many) over a rainbow.
                    //and/or have one color for "damage" and one for "no damage".
                    //or color by the HitState's own type.
                    //color.r = MIN(hits->recordedDamage [iHit] * 8, 255);
                    //color.g = MIN(hits->recordedHitType[iHit] * 8, 255);
                    //color.b = MIN(hits->sphereIdxs     [iHit] * 8, 255);
                    //color.r = MIN(hits->type * 8, 255);
                    //color.a = 0x7F;
                    s = 255; v = 255;
                    break;
                }
            }
            color = hsv2rgb(MIN((int)hits->type * 8, 255), s, v, 128);
        }
        drawSphere(pos->pos, pos->radius, color);
    }
}

void drawAttachPoints(ObjInstance *obj) {
    for(int iAttach=0; iAttach < obj->file->nAttachPoints; iAttach++) {
        AttachPoint *point = &obj->file->pAttachPoints[iAttach];
        Color4b color = {0, 255, 0, 128};
        vec3f pos = point->pos;
        objGetAttachPointWorldPos(obj, iAttach, &pos.x, &pos.y, &pos.z, 0);
        pos.x -= playerMapOffsetX;
        pos.z -= playerMapOffsetZ;
        vec3s rot = point->rot;

        ObjInstance *obj2 = obj;
        while(obj2) {
            rot.x += obj2->pos.rotation.x;
            rot.y += obj2->pos.rotation.y;
            rot.z += obj2->pos.rotation.z;
            obj2 = obj2->parent;
        }
        drawArrow(pos, rot, 2, color);
    }
}

void drawFocusPoints(ObjInstance *obj) {
    for(int i=0; i<obj->file->numFocusPoints; i++) {
        Color4b color = {255, 0, 0, 128};
        vec3f pos = obj->focusPoints[i];
        pos.x -= playerMapOffsetX;
        pos.z -= playerMapOffsetZ;
        pos.x += obj->pos.pos.x;
        pos.y += obj->pos.pos.y;
        pos.z += obj->pos.pos.z;
        drawSphere(pos, 1, color);
    }
}

void drawUnkPoints(ObjInstance *obj) {
    for(int i=0; i<obj->file->numVecs; i++) {
        Color4b color = {0, 0, 255, 128};
        vec3f pos = obj->pVecs[i];
        pos.x -= playerMapOffsetX;
        pos.z -= playerMapOffsetZ;
        pos.x += obj->pos.pos.x;
        pos.y += obj->pos.pos.y;
        pos.z += obj->pos.pos.z;
        drawSphere(pos, 1, color);
    }
}

void _renderObjHitboxes(ObjInstance *obj) {
    //XXX this is still rendering ghosts
    if(obj->flags_0xb0 & (
        ObjInstance_FlagsB0_IsFreed |
        ObjInstance_FlagsB0_Invisible |
        ObjInstance_FlagsB0_DontUpdate)) return;
    if(obj->flags_0xaf & 0x28) return;
    if(obj->pos.flags & ObjInstance_Flags06_Invisible) return;
    if(debugRenderFlags & DEBUGRENDER_HITBOXES) drawHitbox(obj);
    if(debugRenderFlags & DEBUGRENDER_ATTACH_POINTS) drawAttachPoints(obj);
    if(debugRenderFlags & DEBUGRENDER_FOCUS_POINTS) drawFocusPoints(obj);
    if(debugRenderFlags & DEBUGRENDER_UNK_POINTS) drawUnkPoints(obj);

    //recurse into children
    Model *model = obj->models[obj->curModel];
    for(int iChild=0; iChild < obj->nChildren; iChild++) {
        ObjInstance *child = obj->child[iChild];
        _renderObjHitboxes(child);
    }
}
