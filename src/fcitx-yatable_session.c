#include "fcitx-yatable_session.h"

CONFIG_BINDING_BEGIN(YaTableInfo);
CONFIG_BINDING_REGISTER("YaTableConfig", "Enable", Enable);
CONFIG_BINDING_REGISTER("YaTableConfig", "id", id);
CONFIG_BINDING_REGISTER("YaTableConfig", "DisplayName", DisplayName);
CONFIG_BINDING_REGISTER("YaTableConfig", "Priority", YaTableIndex);
CONFIG_BINDING_REGISTER("YaTableConfig", "LangCode", LangCode);
CONFIG_BINDING_REGISTER("YaTableConfig", "Database", userdata);
CONFIG_BINDING_REGISTER("YaTableConfig", "CodeAllmatch", CodeAllmatch);
CONFIG_BINDING_REGISTER("YaTableConfig", "PhraseCodeNoempty",
                        PhraseCodeNoempty);
CONFIG_BINDING_REGISTER("YaTableConfig", "PhraseCodeUseonce",
                        PhraseCodeUseonce);
CONFIG_BINDING_END();
CONFIG_DESC_DEFINE(FcitxYaTableConfigDesc, "yatable.desc");

FcitxYaTableInfo* FcitxYaTableGetAllCFG()
{
    FcitxYaTableInfo* head = NULL,* prev = NULL,* cur = NULL;
    FcitxStringHashSet* configfiles = FcitxXDGGetFiles("yatable", NULL,
                                      ".conf");

    HASH_FOREACH(file, configfiles, FcitxStringHashSet) {
        FcitxConfigFileDesc* cfgdesc = FcitxYaTableConfigDesc();
        if(cfgdesc == NULL) continue;

        FILE* fcfg = FcitxXDGGetFileWithPrefix("yatable", file->name, "r",
                                               NULL);
        if(fcfg == NULL) continue;

        FcitxConfigFile* cfgfile = FcitxConfigParseConfigFileFp(fcfg, cfgdesc);

        cur = fcitx_utils_malloc0(sizeof(FcitxYaTableInfo));
        cur->next = NULL;
        if(head == NULL) {
            head = cur;
        } else {
            prev->next = cur;
        }
        prev = cur;

        YaTableInfoConfigBind(&(cur->info), cfgfile, cfgdesc);
        FcitxConfigBindSync((FcitxGenericConfig*) &(cur->info));
        FcitxConfigFreeConfigFile(cfgfile);
        fclose(fcfg);

    }
    return head;
}

void FcitxYaTableFreeAllCFG(FcitxYaTableInfo* head)
{
    FcitxYaTableInfo* cur = head;
    FcitxYaTableInfo* prev = NULL;

    while(cur != NULL) {
        prev = cur;
        cur = cur->next;
        fcitx_utils_free(prev);
    }
}

FcitxYaTable* FcitxYaTableStartSession(char* sharedir, char* userdir,
                                       char* dbname, YaTableAPI* api,
                                       FcitxInstance* owner)
{
    YaTableInfo* info = (YaTableInfo*)fcitx_utils_malloc0(sizeof(YaTableInfo));
    fcitx_utils_alloc_cat_str(info->sharedata, sharedir, dbname);
    fcitx_utils_alloc_cat_str(info->userdata, userdir, dbname);

    FcitxYaTable* yatable =
        (FcitxYaTable*) fcitx_utils_malloc0(sizeof(FcitxYaTable));

    yatable->sid = api->startsession(info);

    if(yatable->sid == NULL) {
        FcitxLog(ERROR, "Failed to load DATABASE:%s", dbname);
        fcitx_utils_free(info->sharedata);
        fcitx_utils_free(info->userdata);
        fcitx_utils_free(info);
        fcitx_utils_free(yatable);
        return NULL;
    }
    yatable->info = info;
    yatable->api = api;
    yatable->owner = owner;

    size_t numkeychoose = info->num_keyselect;
    char* keychoose = fcitx_utils_malloc0(numkeychoose + 1);
    yatable->keychoose = keychoose;
    YaTableKeyInfo* keyinfo = yatable->info->keyselect;
    size_t i = 0;
    for(i = 0; i < numkeychoose; i++) {
        keychoose[i] = (keyinfo + i)->keyname;
    }

    FcitxInstanceRegisterIM(yatable->owner, yatable, info->id,
                            info->DisplayName, info->id,
                            FcitxYaTableIMInit, FcitxYaTableIMResetIM,
                            FcitxYaTableIMDoInput, FcitxYaTableIMGetCandWords,
                            NULL, FcitxYaTableIMSave,
                            FcitxYaTableIMReloadConfig, NULL,
                            yatable->info->YaTableIndex,
                            yatable->info->LangCode);

    return yatable;
}

