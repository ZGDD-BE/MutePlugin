#include "pch.h"
#include <EventAPI.h>
#include <LoggerAPI.h>
#include <MC/Level.hpp>
#include <MC/BlockInstance.hpp>
#include <MC/Block.hpp>
#include <MC/BlockSource.hpp>
#include <MC/Actor.hpp>
#include <MC/Player.hpp>
#include <MC/ItemStack.hpp>
#include <LLAPI.h>
#include <RegCommandAPI.h>
#include <MC/ServerPlayer.hpp>
#include <vector>
#include <MC/BlockPos.hpp>
#include <third-party/SQLiteCpp/Database.h>
#include <PlayerInfoAPI.h>
#include <direct.h>
#include <third-party/SQLiteCpp/SQLiteCpp.h>
#include <random>

static std::unique_ptr<SQLite::Database> Mute;

#include <RegCommandAPI.h>
Logger logger("Mute");

bool initDB() {
	try {
		Mute = std::make_unique<SQLite::Database>("plugins\\Mute\\Mute.db", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
	}
	catch (std::exception const& e) {
		string folderPath = "plugins\\Mute";
		_mkdir(folderPath.c_str());
		logger.warn("未检测到必要文件，已初始化！");
		Mute = std::make_unique<SQLite::Database>("plugins\\Mute\\Mute.db", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
	}
	Mute->exec("PRAGMA journal_mode = MEMORY");
	Mute->exec("PRAGMA synchronous = NORMAL");
	Mute->exec("CREATE TABLE IF NOT EXISTS Mute (  XUID	TEXT	PRIMARY	KEY	UNIQUE	NOT	NULL,TIMES	INT	NOT	NULL,TIMESTAMPS	INT	NOT	NULL)WITHOUT ROWID; ");
	return true;
}

class MuteCommand : public Command {
	enum MuteCommandOP : int
	{
		add = 1,
		remove = 2,
		addmin = 3
	} op;
	string inpu;
	int inpuint;
public:
	void execute(CommandOrigin const& ori, CommandOutput& output) const override {
		switch (op)
		{
		case MuteCommand::add: {
			if (Mute->execAndGet("select count(*) from Mute where XUID='" + PlayerInfo::getXuid(inpu) + "'").getInt() == 0)
			{
				Mute->exec("INSERT INTO Mute(XUID,TIMES,TIMESTAMPS) VALUES('" + PlayerInfo::getXuid(inpu) + "'," + std::to_string(inpuint) + "," + std::to_string(time(NULL)) + ")");
				output.success("玩家：" + inpu + "已被禁言，时间：" + std::to_string(inpuint) + "s");
			}
			else
			{
				Mute->exec("delete from Mute where XUID='" + PlayerInfo::getXuid(inpu) + "'");
				Mute->exec("VACUUM");
				Mute->exec("INSERT INTO Mute(XUID,TIMES,TIMESTAMPS) VALUES('" + PlayerInfo::getXuid(inpu) + "'," + std::to_string(inpuint) + "," + std::to_string(time(NULL)) + ")");
				output.success("玩家：" + inpu + "已被更新禁言时长，时间：" + std::to_string(inpuint) + "min");
			}
			break;
		}
		case MuteCommand::addmin: {
			int inputmin = inpuint * 60;
			if (Mute->execAndGet("select count(*) from Mute where XUID='" + PlayerInfo::getXuid(inpu) + "'").getInt() == 0)
			{
				Mute->exec("INSERT INTO Mute(XUID,TIMES,TIMESTAMPS) VALUES('" + PlayerInfo::getXuid(inpu) + "'," + std::to_string(inputmin) + "," + std::to_string(time(NULL)) + ")");
				output.success("玩家：" + inpu + "已被禁言，时间：" + std::to_string(inpuint) + "min");
			}
			else
			{
				Mute->exec("delete from Mute where XUID='" + PlayerInfo::getXuid(inpu) + "'");
				Mute->exec("VACUUM");
				Mute->exec("INSERT INTO Mute(XUID,TIMES,TIMESTAMPS) VALUES('" + PlayerInfo::getXuid(inpu) + "'," + std::to_string(inputmin) + "," + std::to_string(time(NULL)) + ")");
				output.success("玩家：" + inpu + "已被更新禁言时长，时间：" + std::to_string(inpuint) + "s");
			}
			break;
		}
		case MuteCommand::remove: {
			if (Mute->execAndGet("select count(*) from Mute where XUID='" + PlayerInfo::getXuid(inpu) + "'").getInt() != 0)
			{
				Mute->exec("delete from Mute where XUID='" + PlayerInfo::getXuid(inpu) + "'");
				Mute->exec("VACUUM");
				output.success("玩家：" + inpu + "已被解除禁言");
			}
			else
			{
				output.error("玩家：" + inpu + "并没有被禁言！");
			}
		}
								break;
		default:
			break;
		}
		return;
	}
	static void setup(CommandRegistry* registry) {
		using RegisterCommandHelper::makeMandatory;
		using RegisterCommandHelper::makeOptional;
		registry->registerCommand("mute", "禁言", CommandPermissionLevel::GameMasters, { (CommandFlagValue)0 }, { (CommandFlagValue)0x80 });
		registry->addEnum<MuteCommandOP>("MuteOptional", { {"add", MuteCommandOP::add},{"addmin", MuteCommandOP::addmin} });
		registry->addEnum<MuteCommandOP>("MuteOptional1", { {"remove", MuteCommandOP::remove} });
		registry->registerOverload<MuteCommand>("mute", makeMandatory<CommandParameterDataType::ENUM>(&MuteCommand::op, "optional", "MuteOptional"), makeMandatory(&MuteCommand::inpu, "player"), makeMandatory(&MuteCommand::inpuint, "time"));
		registry->registerOverload<MuteCommand>("mute", makeMandatory<CommandParameterDataType::ENUM>(&MuteCommand::op, "optional", "MuteOptional1"), makeMandatory(&MuteCommand::inpu, "player"));
	}
};


#include <MC/SignBlock.hpp>
#include <MC/BlockLegacy.hpp>
void entry()
{
	LL::registerPlugin("Mute", "Mute", LL::Version(1, 0, 0));
	Event::PlayerPlaceBlockEvent::subscribe([](Event::PlayerPlaceBlockEvent e)-> bool {
		if (e.mBlockInstance.getBlock()->getLegacyBlock().getBlockEntityType() == 4) {
			string xuid = e.mPlayer->getXuid();
			if (Mute->execAndGet("select count(*) from Mute where XUID='" + xuid + "'").getInt() == 1)
			{
				int Time = Mute->execAndGet("select TIMES from Mute where XUID='" + xuid + "'").getInt();
				int beforeTIMESTAMPS = Mute->execAndGet("select TIMESTAMPS from Mute where XUID='" + xuid + "'").getInt();
				int TIMESTAMPS = time(NULL);
				if (beforeTIMESTAMPS + Time < TIMESTAMPS)
				{
					Mute->exec("delete from Mute where XUID='" + xuid + "'");
					Mute->exec("VACUUM");
					return true;
				}
				else
				{
					e.mPlayer->sendText("您已被禁言！", TextType::RAW);
					return false;
				}
			}
		}
		return true;
		});
	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent& ev) -> bool {
		initDB();
		logger.info << "Mute-禁言插件加载完毕！作者@dofes (qq:234222913)" << logger.endl;
		logger.info << "此插件为@1686296077的定制插件，感谢支持！" << logger.endl;
		logger.info << "欢迎加群：339223711反馈bug或友好交流！" << logger.endl;
		return true;
		});
	Event::PlayerCmdEvent::subscribe([](Event::PlayerCmdEvent e)-> bool {
		string xuid = e.mPlayer->getXuid();
		if (Mute->execAndGet("select count(*) from Mute where XUID='" + xuid + "'").getInt() == 1)
		{
			string str1 = e.mCommand.substr(0, 1);
			string str2 = e.mCommand.substr(0, 2);
			string str3 = e.mCommand.substr(0, 3);
			string str4 = e.mCommand.substr(0, 4);
			if (str3 == "msg" || str2 == "me" || str1 == "w" || str4 == "tell")
			{
				int Time = Mute->execAndGet("select TIMES from Mute where XUID='" + xuid + "'").getInt();
				int beforeTIMESTAMPS = Mute->execAndGet("select TIMESTAMPS from Mute where XUID='" + xuid + "'").getInt();
				int TIMESTAMPS = time(NULL);
				if (beforeTIMESTAMPS + Time < TIMESTAMPS)
				{
					Mute->exec("delete from Mute where XUID='" + xuid + "'");
					Mute->exec("VACUUM");
					str1.clear();
					str2.clear();
					str3.clear();
					str4.clear();
					return true;
				}
				else
				{
					e.mPlayer->sendText("您已被禁言！", TextType::RAW);
					str1.clear();
					str2.clear();
					str3.clear();
					str4.clear();
					return false;
				}
			}
		}
		});
	Event::RegCmdEvent::subscribe([](Event::RegCmdEvent ev) { //注册指令事件
		MuteCommand::setup(ev.mCommandRegistry);
		return true;
		});
}

#include <Utils/NetworkHelper.h>
#include <MC/NetworkPeer.hpp>
#include <MC/NetworkHandler.hpp>
#include <MC/ReadOnlyBinaryStream.hpp>
#include <MC/NetworkIdentifier.hpp>
#include <MC/MinecraftPackets.hpp>
#include <MC/Packet.hpp>
#include <MC/Minecraft.hpp>
THook(
	NetworkPeer::DataStatus,
	"?receivePacket@Connection@NetworkHandler@@QEAA?AW4DataStatus@NetworkPeer@@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAV2@AEBV?$shared_ptr@V?$time_point@Usteady_clock@chrono@std@@V?$duration@_JU?$ratio@$00$0DLJKMKAA@@std@@@23@@chrono@std@@@6@@Z",
	__int64 _this, std::string& data) {
	auto status = original(_this, data);

	if (status == NetworkPeer::DataStatus::OK) {
		auto    stream = ReadOnlyBinaryStream(data, 0i64);
		auto    pktid = stream.getUnsignedVarInt();
		auto    pkthash = do_hash(data.c_str());
		auto    pkttime = _time64(0);
		Packet* pkt;
		SymCall(
			"?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
			void*, Packet**, int)(&pkt, pktid);
		switch ((int)pkt->getId()) {
		case 135:
		case 144:
		case 175:
			return status;
		default:
			break;
		}
		//logger.info("[Network][I][{}]\tLength:{}\tPkt:{}({})", pkttime, data.length(), pkt->getName(), pkt->getId());
		if (pkt->getName() == "TextPacket")
		{
			auto pl = Global<ServerNetworkHandler>->getServerPlayer(*(NetworkIdentifier*)_this);
			string xuid = pl->getXuid();
			//logger.info << xuid << logger.endl;
			//logger.info << Mute->execAndGet("select count(*) from Mute where XUID='" + xuid + "'").getInt() << logger.endl;
			if (Mute->execAndGet("select count(*) from Mute where XUID='" + xuid + "'").getInt() == 1) {
				int Time = Mute->execAndGet("select TIMES from Mute where XUID='" + xuid + "'").getInt();
				int beforeTIMESTAMPS = Mute->execAndGet("select TIMESTAMPS from Mute where XUID='" + xuid + "'").getInt();
				int TIMESTAMPS = time(NULL);
				if (beforeTIMESTAMPS + Time < TIMESTAMPS)
				{
					Mute->exec("delete from Mute where XUID='" + xuid + "'");
					Mute->exec("VACUUM");
					return NetworkPeer::DataStatus::BUSY;
				}
				else
				{
					pl->sendText("您已被禁言！", TextType::RAW);
					return NetworkPeer::DataStatus::BUSY;
				}
			}
			//logger.info("{}",pkt->getId());
		}
	}
	return status;
}/*
#include <MC/ItemStackNetManagerServer.hpp>
#include <mc/ItemStackRequestActionHandler.hpp>
#define _QWORD unsigned long long
#include <MC/ItemStackRequestHandlerSlotInfo.hpp>

 THook(char, "?handleRequestAction@ItemStackRequestActionHandler@@QEAA?AW4ItemStackNetResult@@AEBVItemStackRequestAction@@@Z",
	 ItemStackRequestActionHandler* a1,
	 bool* a2) {
	 __int64 v11[9];
	 memset(v11, 0, sizeof(v11));
	 logger.info(((ItemStack*)v11)->getTypeName());
	 return false;
 }*/
