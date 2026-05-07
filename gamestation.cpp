#include "common/macros.h"
#include "common/nndef.h"
#include "common/nnlogic.h"
#include "gameroot.h"
#include "logic/clientlogic/core/gamestation.h"
#include "utils/tarslog.h"
#include "context/context.h"
#include "config/gameconfig.h"
#include "ddz.pb.h"
#include "process/process.h"
#include "message/sendclientmessage.h"
#include "logic/gamelogic/core/endtimer.h"
#include "CommonCode.pb.h"

using namespace nndef;

namespace game
{
    namespace logic
    {
        namespace clientlogic
        {
            void GameStation(long uid, const vector<char> &vecMsgData, GameRoot *root)
            {
                PERFSTATS_ENTRY();
                __TRY__
                using namespace context;
                using namespace process;
                using namespace config;
                using namespace message;
                using namespace nnlogic;

                DLOG_TRACE("roomid:" << root->roomid() << ", game station. uid: " << uid);
               
                XGameDDZProto::DDZ_msg2csGameStation shcm;
                shcm.set_iresultid(0);
                shcm.set_ibankercid(root->con->getBankerCid());
                shcm.set_itokencid(root->con->getTokenCid());
                shcm.set_lbasescore(root->cfg->getBaseScore());
                shcm.set_ibetid(root->cfg->getBlindLevel());
                shcm.set_imultiple(root->con->getBankerScore());
                if(root->pro->getProcess() == NN_STATE_DAPAI)
                {
                    for(auto card : root->con->getVecCommCard())
                    {
                        shcm.add_scommcards(card);
                    }
                }

                (*shcm.mutable_mcalinfo()).set_bdismiss(root->pro->getProcess() == nil_nnstate);

                XGameDDZProto::DDZ_msg2sActNotify cm;
                cm.set_icid(root->con->getTokenCid());
                cm.set_iacttype(root->pro->getProcess() - 1);
                cm.set_bfirstdapai(root->con->isFirstDaPai());
                
                int cfg_time = root->cfg->getDecisionTime(root->pro->getProcess() - 1);
                int remain_time = cfg_time - (time(nullptr) - root->con->getOptionTime());
                remain_time = remain_time < 0 || remain_time > cfg_time ? 0 : remain_time;

                DLOG_TRACE("roomid:" << root->roomid() << ", cfg_time: " << cfg_time << ", op_time: "<< root->con->getOptionTime() << ", remain_time: "<< remain_time);
                cm.set_icfgoptiontime(cfg_time);
                cm.set_iremainoptiontime(remain_time);

                User* user = root->con->getUserByCid(root->con->getTokenCid());
                if(user)
                {
                    if(root->pro->getProcess() == NN_STATE_DAPAI)
                    {
                        cm.set_bfirstdapai(root->con->isFirstDaPai());
                        cm.set_bcandapai(user->isCanDaPai());
                        auto last_decision = root->con->getLastDecision();
                        for(auto decision : last_decision)
                        {
                            for(auto card : decision.second)
                            {
                                cm.add_sprecards(card);
                            }
                        }
                        cm.set_iprecardtype(root->con->getLastCardType());
                    }
                }
                
                (*shcm.mutable_maction()) = cm;

                std::map<cid_t, User> const &usermap = root->con->getUserMap();
                for (auto it = usermap.begin(); it != usermap.end(); it++)
                { 
                    XGameDDZProto::PlayerInfo userinfo;
                    userinfo.set_icid(it->first);
                    userinfo.set_lplayerid(it->second.getUid());
                    userinfo.set_snickname(it->second.getNick());
                    userinfo.set_sheadstr(it->second.getUrl());
                    userinfo.set_iplayergender(it->second.getGender());
                    userinfo.set_lcidscore(it->second.getWealth());
                    userinfo.set_ijiaodizhu(it->second.getJiaoDiZhu());

                    long ready_time = 30 - (time(nullptr) - it->second.getReadyGameTime());
                    ready_time = ready_time > 30 || ready_time < 0 ? 0 : ready_time;
                    userinfo.set_ireadytime(ready_time);
                    
                    if(uid == it->second.getUid())
                    {
                        for(auto card : it->second.getVecCards())
                        {
                            userinfo.add_shdcards(card);
                        }

                        User* tuser = root->con->getUserByUid(uid);
                        if(tuser)
                        {
                            tuser->setSelfLevel(false);
                            tuser->setLeft(false); 
                            if(tuser->isTuoGuan())
                            {
                                tuser->setTuoGuan(false);
                                tuser->setTimeOut(0);
     
                                XGameDDZProto::DDZ_msg2csTuoGuan tg;
                                tg.set_iresultid(0);
                                tg.set_btuoguan(tuser->isTuoGuan());
                                tg.set_icid(tuser->getCid());
                                sendAllClientMessage<XGameDDZProto::DDZ_msg2csTuoGuan>(XGameDDZProto::DDZ_msg2csTuoGuan_E, tg, root); 
                            }
                            
                        }
                    }
                    else
                    {
                        for(unsigned int i = 0; i < it->second.getVecCards().size(); i++)
                        {
                            userinfo.add_shdcards(-1);
                        } 
                    }
                    userinfo.set_btuoguan(it->second.isTuoGuan());

                    for(auto card : it->second.getVecOutCards())
                    {
                        userinfo.add_soutcards(card);
                    }

                    (*shcm.mutable_muserinfo())[it->first] = userinfo;
                }

                DLOG_TRACE("roomid:" << root->roomid() << ", game station. uid: " << uid << ", shcm: "<< logPb(shcm));
                sendClientMessage<XGameDDZProto::DDZ_msg2csGameStation>(uid, XGameDDZProto::DDZ_msg2csGameStation_E, shcm, root);

                __CATCH__
                PERFSTATS_EXIT();
            }
        }
    }
}
