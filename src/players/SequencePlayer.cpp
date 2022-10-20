#include "SequencePlayer.h"

#include <QDir>

void SequencePlayer::LoadConfigs(QString const& configPath)
{
	LoadOutputs(configPath + QDir::separator() + "xlights_networks.xml");
}

void SequencePlayer::LoadSequence(QString const& sequencePath, QString const& mediaPath)
{

}

void SequencePlayer::PlaySequence()
{

}

void SequencePlayer::StopSequence()
{

}

void SequencePlayer::LoadOutputs(QString const& configPath)
{
	m_outputManager = std::make_unique<OutputManager>();
	connect(m_outputManager.get(), &OutputManager::AddController, this, &SequencePlayer::on_AddController);
	m_outputManager->LoadOutputs(configPath);
}