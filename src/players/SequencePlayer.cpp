#include "SequencePlayer.h"

#include <QDir>

SequencePlayer::SequencePlayer():
	m_mediaPlayer(std::make_unique<QMediaPlayer>())
{
	memset(m_seqData, 0, sizeof(m_seqData));
}

void SequencePlayer::LoadConfigs(QString const& configPath)
{
	LoadOutputs(configPath + QDir::separator() + "xlights_networks.xml");
}

void SequencePlayer::LoadSequence(QString const& sequencePath, QString const& mediaPath)
{
	bool loaded = LoadSeqFile(sequencePath);

	if(!loaded)
	{
		//failed to load
		return;
	}
	QFileInfo seqInfo(sequencePath);
	m_seqFileName = seqInfo.fileName();

	if(!mediaPath.isEmpty())
	{
		m_mediaFile = mediaPath;
	}
	
	if(!m_mediaFile.isEmpty() && QFile::exists(m_mediaFile))
	{
		m_seqType = SeqType::Music;
	}
	else
	{
		m_seqType = SeqType::Animation;
	}
	PlaySequence();
}

void SequencePlayer::PlaySequence()
{
	m_outputManager->OpenOutputs();
	//start timer
	m_lastFrameRead = 0;
	m_lastFrameData = m_seqFile->getFrame(m_lastFrameRead);
	if(SeqType::Animation == m_seqType)
	{
		StartAnimationSeq();
	}
	else if(SeqType::Music == m_seqType)
	{
		StartMusicSeq();
	}
}

void SequencePlayer::StopSequence()
{
	m_playbackThread.requestInterruption();
	m_playbackThread.quit();
	m_playbackThread.wait();
	//stop timer
	m_outputManager->CloseOutputs();
}

void SequencePlayer::TriggerOutputData()
{
	m_lastFrameData->readFrame((uint8_t*)m_seqData, FPPD_MAX_CHANNELS);
	m_outputManager->OutputData((uint8_t*)m_seqData);
	m_lastFrameRead++;
	if(m_lastFrameRead >= m_numberofFrame)
	{
		StopSequence();
		return;
	}
	m_lastFrameData = m_seqFile->getFrame(m_lastFrameRead);
	if((m_lastFrameRead * m_seqStepTime) % 1000 == 0)
	{
		emit UpdateTime(m_seqFileName, m_lastFrameRead * m_seqStepTime, m_seqMSDuration );
	}
}

void SequencePlayer::LoadOutputs(QString const& configPath)
{
	m_outputManager = std::make_unique<OutputManager>();
	connect(m_outputManager.get(), &OutputManager::AddController, this, &SequencePlayer::on_AddController);
	connect(m_outputManager.get(), &OutputManager::SetChannelCount, this, &SequencePlayer::setTotalChannels);
	if(m_outputManager->LoadOutputs(configPath))
	{
		emit UpdateStatus("Loaded: " + configPath);
	}
}

bool SequencePlayer::LoadSeqFile(QString const& sequencePath)
{
	m_seqFile = nullptr;
    FSEQFile* seqFile = FSEQFile::openFSEQFile(sequencePath.toStdString());
    if (seqFile == nullptr) 
	{
        return false;
    }
	m_seqStepTime = seqFile->getStepTime();

	m_mediaFile = QString::fromStdString(seqFile->getMediaFilename());

	m_seqFile = seqFile;

	m_seqMSDuration = seqFile->getNumFrames() * seqFile->getStepTime();
    m_numberofFrame = seqFile->getNumFrames();
	return true;
}

void SequencePlayer::StartAnimationSeq()
{
	m_playbackTimer = std::make_unique<QTimer>(this);
    m_playbackTimer->setTimerType(Qt::PreciseTimer);
    m_playbackTimer->setInterval(m_seqStepTime);

    //m_playbackThread = std::make_unique<QThread>(this);
    moveToThread(&m_playbackThread);
    m_playbackTimer->moveToThread(&m_playbackThread);
    //this->moveToThread(thread);

    connect(&m_playbackThread, SIGNAL(started()), m_playbackTimer.get(), SLOT(start()));
    connect(m_playbackTimer.get(), SIGNAL(timeout()), this, SLOT(TriggerOutputData()));
    connect(this, SIGNAL(finished()), m_playbackTimer.get(), SLOT(stop()));
    connect(this, SIGNAL(finished()), &m_playbackThread, SLOT(quit()));

	emit UpdateStatus("Playing " + m_seqFileName);
    m_playbackThread.start();
}

void SequencePlayer::on_durationChanged(qint64 )
{
	TriggerOutputData();
}

void SequencePlayer::StartMusicSeq()
{
	m_mediaPlayer = std::make_unique<QMediaPlayer>();
	m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_mediaFile));
	m_mediaPlayer->setNotifyInterval(m_seqStepTime);
	connect(m_mediaPlayer.get(), &QMediaPlayer::durationChanged, this, &SequencePlayer::on_durationChanged);
	emit UpdateStatus("Playing " + m_seqFileName);
	m_mediaPlayer->play();
}