#include "fcitx-yatable.h"
#include "fcitx-yatable_session.h"

static void* FcitxYaTableCreate(FcitxInstance* instance);
static void FcitxYaTableDestroy(void* arg);

FCITX_EXPORT_API
FcitxIMClass ime = {
    FcitxYaTableCreate,
    FcitxYaTableDestroy
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static void* FcitxYaTableCreate(FcitxInstance* instance)
{

    FcitxXDGMakeDirUser("yatable");
    YaTableAPI* api = YaTableGetAPIs();
    if(api == NULL) {
        FcitxLog(ERROR, "Failed to YaTableGetAPIs");
        return NULL;
    }

    size_t userdirlen = 0;
    char** userdir = FcitxXDGGetPathUserWithPrefix(&userdirlen, "yatable");
    char* pkgdir = fcitx_utils_get_fcitx_path("pkgdatadir");

    char* yatableuserdir = NULL,* yatablesharedir = NULL;
    fcitx_utils_alloc_cat_str(yatableuserdir, userdir[0], "/");
    fcitx_utils_alloc_cat_str(yatablesharedir, pkgdir, "/yatable/");

    FcitxXDGFreePath(userdir);
    fcitx_utils_free(pkgdir);
    FcitxYaTable* head = NULL,* cur = NULL,* prev = NULL;
    FcitxYaTableInfo* cfghead = FcitxYaTableGetAllCFG();
    FcitxYaTableInfo* cfgcur = cfghead,* cfgprev= NULL;

    while(cfgcur != NULL) {
        cur = FcitxYaTableStartSession(yatablesharedir, yatableuserdir,
                                       cfgcur->info.userdata, api, instance);
        cfgprev = cfgcur;
        cfgcur = cfgcur->next;
        if(cur == NULL) continue;
        cur->next = NULL;
        if(head == NULL) {
            head = cur;
        } else {
            prev->next = cur;
        }
        prev = cur;

        api->configsetbool(prev->sid, "Enable", cfgprev->info.Enable);
        api->configsetstring(prev->sid, "id", cfgprev->info.id);
        api->configsetstring(prev->sid, "DisplayName",
                             cfgprev->info.DisplayName);
        api->configsetint(prev->sid, "YaTableIndex",
                          cfgprev->info.YaTableIndex);
        api->configsetstring(prev->sid, "LangCode", cfgprev->info.LangCode);
        api->configsetbool(prev->sid, "CodeAllmatch",
                           cfgprev->info.CodeAllmatch);
        api->configsetbool(prev->sid, "PhraseCodeNoempty",
                           cfgprev->info.PhraseCodeNoempty);
        api->configsetbool(prev->sid, "PhraseCodeUseonce",
                           cfgprev->info.PhraseCodeUseonce);
    }
    FcitxYaTableFreeAllCFG(cfghead);

    fcitx_utils_free(yatableuserdir);
    fcitx_utils_free(yatablesharedir);

    return head;
}

static void FcitxYaTableDestroy(void* arg)
{
    FcitxYaTable* head = (FcitxYaTable*) arg;
    FcitxYaTable* cur = head,* prev = NULL;

    while(cur != NULL) {
        prev = cur;
        cur = cur->next;
        FcitxInstanceUnregisterIM(prev->owner, prev->info->id);
        head->api->cleansession(prev->sid);
        fcitx_utils_free(prev->info->sharedata);
        fcitx_utils_free(prev->info->userdata);
        fcitx_utils_free(prev->info);
        fcitx_utils_free(prev->keychoose);
        fcitx_utils_free(prev);
    }
}

boolean FcitxYaTableIMInit(void* arg)
{
    return true;
}

void FcitxYaTableIMResetIM(void* arg)
{
    FcitxYaTable* yatable = (FcitxYaTable*) arg;
    YaTableAPI* api = yatable->api;
    api->contextcleancontext(yatable->context);
    yatable->context = NULL;
    api->commitclean(yatable->sid);
}

static INPUT_RETURN_VALUE FcitxYaTableIMDoInputReal(void* arg, FcitxKeySym sym,
        unsigned int state)
{
    FcitxYaTable* yatable = (FcitxYaTable*) arg;
    YaTableSid* sid = yatable->sid;
    YaTableContext* context = yatable->context;
    YaTableAPI* api = yatable->api;

    api->processkey(sid, sym, state);
    char* prevstr = NULL,* commitstr = NULL;
    int index = 0;

    YaTableKeyEvent event = api->keygetkeyevent(sid);
    switch(event) {
    case KEY_EVENT_NOTHING:
        if((api->getcommitbysid(sid))->state == false) {
            return IRV_TO_PROCESS;
        }
        break;
    case KEY_EVENT_COMMIT_RAW:
        commitstr = api->keygetcommitstrbykeyevent(sid, event);
        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(yatable->owner);
        FcitxInstanceCommitString(yatable->owner, ic, commitstr);
    case KEY_EVENT_COMMIT_CLEAR:
        api->contextcleancontext(yatable->context);
        yatable->context = NULL;
        api->commitclean(sid);
        return IRV_DISPLAY_CANDWORDS;
    case KEY_EVENT_CODE:
    case KEY_EVENT_COMMIT_CHANGED:
        prevstr = api->commitgetprevstr(sid);
        if((prevstr == NULL) || (*prevstr == '\0')) {
            api->contextcleancontext(yatable->context);
            yatable->context = NULL;
            break;
        }
        context = api->contextgetcontext(sid, prevstr, true);
        if(context != NULL) {
            api->contextcleancontext(yatable->context);
            yatable->context = context;
        }
        break;
    case KEY_EVENT_SELECT:
        index = api->keygetkeyindex(sid);
    case KEY_EVENT_COMMIT_CURRENT:
        prevstr = api->commitgetprevstr(sid);

        if((event == KEY_EVENT_COMMIT_CURRENT) &&(context != NULL)) {
            YaTableCandInfo* cand = api->contextgetselectedofpage(context);
            index = cand->indexofpage;
        }
        api->commitgenword(sid, yatable->context, index);

        prevstr = api->commitgetprevstr(sid);
        char* nextstr = api->commitgetnextstr(sid);
        if(*prevstr == '\0' && ((nextstr == NULL) || (*nextstr == '\0'))) {
            commitstr = api->keygetcommitstrbykeyevent(sid, event);
            FcitxInputContext* ic = FcitxInstanceGetCurrentIC(yatable->owner);
            FcitxInstanceCommitString(yatable->owner, ic, commitstr);
            api->updatecommitdata(sid);
            api->contextcleancontext(yatable->context);
            yatable->context = NULL;
            api->commitclean(sid);
        } else {
            context = api->contextgetcontext(sid, prevstr, true);
            if(context != NULL)
                api->contextcleancontext(yatable->context);
            yatable->context = context;
        }
        break;
    case KEY_EVENT_PAGE_PREV:
        if(context != NULL) {
            YaTableCandInfo* cand = api->contextgetselectedofpage(context);
            api->contextgetprevpage(context, cand->indexofpage);
        }
        break;
    case KEY_EVENT_PAGE_NEXT:
        if(context != NULL) {
            YaTableCandInfo* cand = api->contextgetselectedofpage(context);
            api->contextgetnextpage(context, cand->indexofpage);
        }
        break;
    case KEY_EVENT_CAND_PREV:
        api->contextselectprev(context);
        break;
    case KEY_EVENT_CAND_NEXT:
        api->contextselectnext(context);
        break;
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxYaTableIMDoInput(void* arg, FcitxKeySym _sym,
        unsigned int _state)
{
    FcitxYaTable* yatable = (FcitxYaTable*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(yatable->owner);
    FcitxKeySym sym = (FcitxKeySym) FcitxInputStateGetKeySym(input);
    uint32_t state = FcitxInputStateGetKeyState(input);

    _state &= (FcitxKeyState_SimpleMask | FcitxKeyState_CapsLock);
    if(_state & (~(FcitxKeyState_Ctrl_Alt_Shift | FcitxKeyState_CapsLock))) {
        return IRV_TO_PROCESS;
    }

    state &= (FcitxKeyState_SimpleMask | FcitxKeyState_CapsLock);
    return FcitxYaTableIMDoInputReal(arg, sym, state);
}

INPUT_RETURN_VALUE FcitxYaTableGetCandWord(void* arg,
        FcitxCandidateWord* candword)
{
    size_t* priv = candword->priv;
    FcitxYaTable* yatable = (FcitxYaTable*) arg;
    YaTableKeyInfo* keyinfo = yatable->info->keyselect;

    FcitxYaTableIMDoInputReal(yatable, (keyinfo + *priv)->keycode, 0);
    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxYaTableIMGetCandWords(void* arg)
{
    FcitxYaTable* yatable = (FcitxYaTable*) arg;
    YaTableContext* context = yatable->context;
    YaTableSid* sid = yatable->sid;
    YaTableAPI* api = yatable->api;

    if((api->getcommitbysid(sid))->state == false) {
        FcitxInstanceCleanInputWindow(yatable->owner);
        FcitxUICloseInputWindow(yatable->owner);
        return IRV_DO_NOTHING;
    }

    FcitxInputState* input = FcitxInstanceGetInputState(yatable->owner);
    FcitxInstanceCleanInputWindow(yatable->owner);
    FcitxMessages* preedit = FcitxInputStateGetPreedit(input);
    FcitxMessages* cliedit = FcitxInputStateGetClientPreedit(input);
    boolean bcliedit = FcitxInstanceICSupportPreedit(yatable->owner,
                       FcitxInstanceGetCurrentIC(yatable->owner));

    FcitxInputStateSetShowCursor(input, true);

    char* commitstr = api->commitgetcommitstr(sid);
    char* prevstr = api->commitgetprevstr(sid);
    char* nextstr = api->commitgetnextstr(sid);
    char* aliasstr = api->commitgetaliasstr(sid);

    size_t inputlen = 0;
    inputlen += (commitstr == NULL? 0:strlen(commitstr));
    inputlen += (prevstr == NULL? 0:strlen(prevstr));

    if(bcliedit) {
        FcitxMessagesAddMessageAtLast(cliedit, MSG_TIPS, "%s",
                                      commitstr == NULL? "":commitstr);
        FcitxInputStateSetClientCursorPos(input, inputlen);
    }
    FcitxMessagesAddMessageAtLast(preedit, MSG_TIPS, "%s",
                                  commitstr == NULL? "":commitstr);
    FcitxInputStateSetCursorPos(input, inputlen);
    FcitxMessagesAddMessageAtLast(preedit, MSG_INPUT, "%s%s",
                                  prevstr == NULL? "":prevstr,
                                  nextstr == NULL? "":nextstr);

    if((aliasstr != NULL) && (*aliasstr != '\0')) {
        FcitxMessagesAddMessageAtLast(preedit, MSG_CODE, "·%s", aliasstr);
    }

    if(context != NULL) {
        FcitxCandidateWordList* list = FcitxInputStateGetCandidateList(input);
        YaTableCandInfo* cand = yatable->context->currentcand;
        if(cand == NULL) return IRV_DISPLAY_CANDWORDS;

        do {
            if(bcliedit && cand->selected) {
                FcitxMessagesAddMessageAtLast(cliedit, MSG_OTHER, "%s",
                                              cand->candword);
            }

            FcitxCandidateWord candword = {0};

            char* compstr = NULL;
            if(cand->compalias != NULL) {
                fcitx_utils_alloc_cat_str(compstr, " 【～",
                                          cand->compalias, "】");
            }

            candword.strWord = strdup(cand->candword);
            candword.wordType = cand->selected? MSG_CANDIATE_CURSOR:
                                MSG_OTHER | ~MSG_CANDIATE_CURSOR;
            candword.strExtra = compstr;
            candword.extraType = MSG_CODE;
            candword.callback = FcitxYaTableGetCandWord;
            candword.owner = yatable;

            size_t* priv = fcitx_utils_new(size_t);
            *priv = cand->indexofpage;
            candword.priv = priv;

            FcitxCandidateWordAppend(list, &candword);
            cand = cand->nextcand;
            if(cand == NULL) break;
        } while(cand->indexofpage != 0);

        size_t candnumofpage = context->candnumofpage;
        size_t candnum = context->candnum;
        FcitxCandidateWordSetPageSize(list, candnumofpage);
        FcitxCandidateWordSetOverridePaging(list,
                                            context->currentpage == 0 ?
                                            false:true,
                                            context->currentpage ==
                                            (candnum / candnumofpage) ?
                                            false:true,
                                            NULL, NULL, NULL);
        FcitxCandidateWordSetChoose(list, yatable->keychoose);
    }

    return IRV_DISPLAY_CANDWORDS;
}

void FcitxYaTableIMSave(void* arg)
{
}

void FcitxYaTableIMReloadConfig(void* arg)
{
}

