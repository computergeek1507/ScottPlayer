#include "OutputManager.h"

#include "DDPOutput.h"
#include "E131Output.h"

#include <QtXml>
#include <QFile>

OutputManager::OutputManager()
{

}

bool OutputManager::OpenOutputs()
{
	for (auto const& o : m_outputs)
	{
		o->Open();
	}
	return true;
}

void OutputManager::OutputData(uint8_t* data)
{
	for (auto const& o : m_outputs)
	{
		o->OutputFrame(data);
	}
}

void OutputManager::LoadOutputs(QString const& outputConfig)
{
	QDomDocument xmlNetworks;
	QFile f(outputConfig);
	if (!f.open(QIODevice::ReadOnly))
	{
		return;
	}
	xmlNetworks.setContent(&f);
	f.close();

	QDomElement rootXML = xmlNetworks.documentElement();

	uint64_t startChannel{ 1 };
	QString Type = rootXML.tagName();
	QString proxy = rootXML.attribute("GlobalFPPProxy", "");
	for (QDomElement controllerXML = rootXML.firstChildElement("Controller"); !controllerXML.isNull(); controllerXML = controllerXML.nextSiblingElement("Controller"))
	{
		for (QDomElement networkXML = controllerXML.firstChildElement("network"); !networkXML.isNull(); networkXML = networkXML.nextSiblingElement("network"))
		{
			QString nType = networkXML.attribute("NetworkType", "");
			QString sChannels = networkXML.attribute("MaxChannels", "0");
			QString ipAddress = networkXML.attribute("ComPort", "");
			QString universe = networkXML.attribute("BaudRate", "");
			uint64_t iChannels =  sChannels.toULong();
			if ("DDP" == nType)
			{
				QString sKeepChannels = networkXML.attribute("KeepChannelNumbers", "1");
				QString sPacketSize = networkXML.attribute("ChannelsPerPacket", "1440");

				auto ddp = std::make_unique<DDPOutput>();
				ddp->IP = ipAddress;
				ddp->PacketSize = sPacketSize.toUInt();
				ddp->KeepChannels = sPacketSize.toInt();
				ddp->StartChannel = startChannel;
				m_outputs.push_back(std::move(ddp));
				emit AddController(nType, ipAddress, sChannels);
			}
			else if ("E131" == nType)
			{
				QString sPacketSize = networkXML.attribute("MaxChannels", "510");
				auto e131 = std::make_unique<E131Output>();
				e131->IP = ipAddress;
				e131->PacketSize = sPacketSize.toUInt();
				e131->Universe = universe.toUInt();
				e131->StartChannel = startChannel;
				m_outputs.push_back(std::move(e131));
				emit AddController(nType, ipAddress, sChannels);
			}
			else
			{
				//unsupported type
			}
			startChannel += iChannels;
		}
	}
}