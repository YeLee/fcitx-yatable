#ifndef FCITX_YATABLE_CONFIG_H
#define FCITX_YATABLE_CONFIG_H
#include "fcitx-yatable.h"

typedef struct _FCITX_YATABLE_INFO_ {
    struct _FCITX_YATABLE_INFO_* next;
    YaTableInfo info;
} FcitxYaTableInfo;

FcitxYaTableInfo* FcitxYaTableGetAllCFG();
void FcitxYaTableFreeAllCFG(FcitxYaTableInfo* head);
FcitxYaTable* FcitxYaTableStartSession(char* sharedir, char* userdir,
                                       char* dbname, YaTableAPI* api,
                                       FcitxInstance* owner);

#endif
