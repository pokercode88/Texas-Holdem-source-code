#include "common/macros.h"
#include "common/nndef.h"
#include "common/nnlogic.h"
#include "gameroot.h"
#include "logic/clientlogic/core/gamestation.h"
#include "logic/gamelogic/core/checkbegin.h"
#include "logic/gamelogic/core/leavedesk.h"
#include "logic/gamelogic/core/begintimer.h"
#include "logic/gamelogic/core/endtimer.h"
#include "utils/tarslog.h"
#include "context/context.h"
#include "config/gameconfig.h"
#include "ddz.pb.h"
#include "process/process.h"
#include "message/sendclientmessage.h"
#include "message/sendroommessage.h"
#include "CommonCode.pb.h"

using namespace nndef;

namespace game
{
    namespace logic
    {
        namespace clientlogic
        {
            void OnReady(long uid, const vector<char> &vecMsgData, GameRoot *root)
            {
                PERFSTATS_ENTRY();
                __TRY__
                using namespace RoomSo;
                using namespace context;
                using namespace process;
                using namespace config;
                using namespace message;
                using namespace nnlogic;
                using namespace gamelogic;

                XGameDDZProto::DDZ_msg2csReady shcm4 = pbToObj<XGameDDZProto::DDZ_msg2csReady>(vecMsgData);
                DLOG_TRACE("roomid:" << root->roomid()<<", uid:"<< uid << ",OnReady. shcm4: "<< logPb(shcm4));

                XGameDDZProto::DDZ_msg2csReady shcm2;
                shcm2.set_iresultid(0);
                User* user = root->con->getUserByUid(uid);
                if(user)
                {
                    shcm2.set_icid(user->getCid());
                    if(user->getWealth() >= root->cfg->getMinTake())
                    {
                        if(user->isReady())
                        {
                            DLOG_TRACE("roomid:" << root->roomid()<<", uid:"<< uid << ",user is ready.");
                            return;
                        }

                        user->setReady(true);
                        shcm2.set_ireadytime(30);

                        DLOG_TRACE("roomid:" << root->roomid()<<", uid:"<< uid << ",OnReady. shcm2: "<< logPb(shcm2));
                        sendAllClientMessage<XGameDDZProto::DDZ_msg2csReady>(XGameDDZProto::DDZ_msg2csReady_E, shcm2, root);

                        if(root->con->getFirstWaitGameCid() == nil_cid)
                        {
                            //30s 倒计时
                            EndTimer(NN_XTIME_GAME_XTIME, root, false);
                            BeginTimer(NN_XTIME_GAME_XTIME, 30, [](TimerParam & param)->int
                            {
                                auto body = static_cast<std::tuple<GameRoot *> const *>(param.getBody());
                                auto root = std::get<0>(*body);

                                DLOG_TRACE("roomid:" << root->roomid() << ", timeout dismiss.");
                                gamelogic::LeaveDesk(root, true);

                                return 0;
                            }, root, false);
                        }

                        user->setReadyGameTime(time(nullptr));

                        //check game begin
                        game::logic::gamelogic::CheckBegin(root);
                    }
                    else
                    {
                        shcm2.set_iresultid(XGameRetCode::ROOM_GOLD_NOT_ENOUGH);
                        sendClientMessage<XGameDDZProto::DDZ_msg2csReady>(uid, XGameDDZProto::DDZ_msg2csReady_E, shcm2, root);
                    }
                }
                else
                {
                    shcm2.set_iresultid(-1);
                    sendClientMessage<XGameDDZProto::DDZ_msg2csReady>(uid, XGameDDZProto::DDZ_msg2csReady_E, shcm2, root);
                }
               
                __CATCH__
                PERFSTATS_EXIT();
            }
        }
    }
}
