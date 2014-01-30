#ifndef FCITX_YATABLE_H
#define FCITX_YATABLE_H

#include <sys/stat.h>
#include <unistd.h>
#include <fcitx/fcitx.h>
#include <fcitx/instance.h>
#include <fcitx/candidate.h>
#include <fcitx/ime.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utils.h>
#include <yatable_api.h>

#define _(x) dgettext("fcitx-yatable", (x))

typedef struct _FCITX_YATABLE_ {
    struct _FCITX_YATABLE_* next;
    YaTableAPI* api;
    YaTableInfo* info;
    FcitxInstance* owner;
    YaTableSid* sid;
    YaTableContext* context;
    char* keychoose;
} FcitxYaTable;

boolean FcitxYaTableIMInit(void* arg);
void FcitxYaTableIMResetIM(void* arg);
INPUT_RETURN_VALUE FcitxYaTableIMDoInput(void* arg, FcitxKeySym _sym,
        unsigned int _state);
INPUT_RETURN_VALUE FcitxYaTableIMGetCandWords(void* arg);
void FcitxYaTableIMSave(void* arg);
void FcitxYaTableIMReloadConfig(void* arg);

#endif
