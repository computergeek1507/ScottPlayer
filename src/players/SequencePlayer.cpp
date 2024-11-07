#include "SequencePlayer.h"

#include <QDir>
#include <QCoreApplication>

SequencePlayer::SequencePlayer():
	m_mediaPlayer(std::make_unique<QMediaPlayer>()),
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	m_audioOutput(std::make_unique<QAudioOutput>()),
#endif

	//m_playbackThread(std::make_unique<QThread>(this)),
	m_playbackTimer(std::make_unique<QTimer>(this)),
	m_logger(spdlog::get("scottplayer"))
{
	memset(m_seqData, 0, sizeof(m_seqData));

	//m_playbackTimer = std::make_unique<QTimer>(this);
	m_playbackTimer->setTimerType(Qt::PreciseTimer);
	m_playbackTimer->setInterval(50);

	//m_playbackThread = std::make_unique<QThread>(this);
	//moveToThread(&m_playbackThread);
	//m_playbackTimer->moveToThread(&m_playbackThread);
	//this->moveToThread(thread);
	m_playbackTimer->moveToThread(&m_playbackThread);

	connect(&m_playbackThread, SIGNAL(started()), m_playbackTimer.get(), SLOT(start()));
	connect(m_playbackTimer.get(), SIGNAL(timeout()), this, SLOT(TriggerOutputData()));
	connect(this, SIGNAL(finished()), m_playbackTimer.get(), SLOT(stop()));
	connect(this, SIGNAL(finished()), &m_playbackThread, SLOT(quit()));


	//m_mediaPlayer = std::make_unique<QMediaPlayer>();
	connect(m_mediaPlayer.get(), &QMediaPlayer::positionChanged, this, &SequencePlayer::TriggerTimedOutputData);
	connect(m_mediaPlayer.get(), &QMediaPlayer::mediaStatusChanged,	this, &SequencePlayer::MediaStatusChanged);
	
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	m_audioOutput->setVolume(1.0);
	m_mediaPlayer->setAudioOutput(m_audioOutput.get());
	connect(m_mediaPlayer.get(), &QMediaPlayer::playbackStateChanged, this, &SequencePlayer::MediaPlaybackStateChanged);
#else
	m_mediaPlayer->setVolume(100);
	connect(m_mediaPlayer.get(), &QMediaPlayer::stateChanged, this, &SequencePlayer::MediaStateChanged);
#endif

	m_syncManager = std::make_unique<SyncManager>(this);
}

SequencePlayer::~SequencePlayer()
{
	m_playbackTimer->stop();
	m_playbackThread.requestInterruption();
	m_playbackThread.quit();
	m_playbackThread.wait();
	delete m_seqFile;
	delete m_lastFrameData;
}

void SequencePlayer::LoadConfigs(QString const& configPath)
{
	m_configFolder = configPath;
	LoadOutputs(configPath + QDir::separator() + "xlights_networks.xml");
}

void SequencePlayer::LoadSequence(QString const& sequencePath, QString const& mediaPath)
{
	emit UpdatePlaybackStatus(sequencePath, PlaybackStatus::Loading);
	bool loaded = LoadSeqFile(sequencePath);

	if(!loaded)
	{
		m_logger->error("Unable to load sequence file: {}", sequencePath.toStdString());
		//failed to load
		return;
	}
	QFileInfo seqInfo(sequencePath);
	m_seqFileName = seqInfo.fileName();

	if(!mediaPath.isEmpty())
	{
		QFileInfo mediaInfo(mediaPath);
		if(!QFile::exists(mediaPath))
		{
			m_logger->warn("Media not in original Location, Looking for Media File: {}", mediaPath.toStdString());
			QDir d = seqInfo.absoluteDir();
			QString absolute = d.absolutePath();
			auto newPath = absolute + QDir::separator() + mediaInfo.fileName();
			if(QFile::exists(newPath))
			{
				m_logger->warn("Media File Found: {}", newPath.toStdString());
				m_mediaFile = newPath;
			}
			else
			{
				auto newConfigPath = m_configFolder + QDir::separator() + mediaInfo.fileName();
				if(QFile::exists(newConfigPath))
				{
					m_logger->warn("Media File Found: {}", newConfigPath.toStdString());
					m_mediaFile = newConfigPath;
				}
			}
		}
		else
		{
			m_mediaFile = mediaPath;
		}
		
		m_mediaName = mediaInfo.fileName();
	}
	
	if(!m_mediaFile.isEmpty())
	{
		if(!QFile::exists(m_mediaFile))
		{
			m_logger->error("Media File Not Found : {}", mediaPath.toStdString());
			m_seqType = SeqType::Animation;
		}
		else
		{
			m_seqType = SeqType::Music;
		}
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
	m_syncManager->OpenOutputs();
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
	m_playbackTimer->stop();
	m_playbackThread.requestInterruption();
	m_playbackThread.quit();
	m_playbackThread.wait();

#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState)
#else
	if (m_mediaPlayer->state() == QMediaPlayer::PlayingState)
#endif
	{
		m_mediaPlayer->stop();
	}

	m_syncManager->SendStop();

	//stop timer
	m_outputManager->CloseOutputs();
	m_syncManager->CloseOutputs();
	//emit UpdateStatus("Sequence Ended " + m_seqFileName);
	emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
}

void SequencePlayer::TriggerOutputData()
{
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	if (SeqType::Music == m_seqType) {
		TriggerTimedOutputData(m_mediaPlayer->position());
		return;
	}
#endif

	//int64_t timeMS = m_lastFrameRead * m_seqStepTime;

	//qDebug() << "O:" << timeMS << "ms";
	m_lastFrameData->readFrame((uint8_t*)m_seqData, FPPD_MAX_CHANNELS);
	m_outputManager->OutputData((uint8_t*)m_seqData);
	SendSync(m_lastFrameRead);
	m_lastFrameRead++;

	if((m_lastFrameRead * m_seqStepTime) % 1000 == 0)
	{
		emit UpdateTime(m_seqFileName, m_lastFrameRead * m_seqStepTime, m_seqMSDuration );
	}

	if(m_lastFrameRead >= m_numberofFrame)
	{
		StopSequence();
		return;
	}
	m_lastFrameData = m_seqFile->getFrame(m_lastFrameRead);
}

void SequencePlayer::TriggerTimedOutputData(qint64 timeMS)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	if (m_mediaPlayer->playbackState() != QMediaPlayer::PlayingState)
#else
	if (m_mediaPlayer->state() != QMediaPlayer::PlayingState)
#endif	
	{
		return;
	}
	int64_t approxFrame = timeMS / m_seqStepTime;

	if (approxFrame >= m_numberofFrame)
	{
		StopSequence();
		return;
	}

	//qDebug() << "O:" << timeMS << "ms";
	m_lastFrameData = m_seqFile->getFrame(approxFrame);
	m_lastFrameData->readFrame((uint8_t*)m_seqData, FPPD_MAX_CHANNELS);
	m_outputManager->OutputData((uint8_t*)m_seqData);
	
	SendSync(approxFrame);
	if ((approxFrame * m_seqStepTime) % 1000 == 0)
	{
		emit UpdateTime(m_seqFileName, approxFrame * m_seqStepTime, m_seqMSDuration);
	}

	
	//m_lastFrameData = m_seqFile->getFrame(m_lastFrameRead);
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

void SequencePlayer::SendSync(qint64 frameIdx)
{
	m_syncManager->SendSync(m_seqStepTime, frameIdx, m_seqFileName, m_mediaName);
}

bool SequencePlayer::LoadSeqFile(QString const& sequencePath)
{
	m_seqFile = nullptr;
	FSEQFile* seqFile = FSEQFile::openFSEQFile(sequencePath.toStdString());
	if (seqFile == nullptr)
	{
		emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
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
	//m_playbackTimer = std::make_unique<QTimer>(this);
    //m_playbackTimer->setTimerType(Qt::PreciseTimer);
    //m_playbackTimer->setInterval(m_seqStepTime);

    //m_playbackThread = std::make_unique<QThread>(this);
    //moveToThread(&m_playbackThread);
    //m_playbackTimer->moveToThread(&m_playbackThread);
    //this->moveToThread(thread);
	//m_playbackTimer->moveToThread( &m_playbackThread );

    //connect(&m_playbackThread, SIGNAL(started()), m_playbackTimer.get(), SLOT(start()));
    //connect(m_playbackTimer.get(), SIGNAL(timeout()), this, SLOT(TriggerOutputData()));
    //connect(this, SIGNAL(finished()), m_playbackTimer.get(), SLOT(stop()));
    //connect(this, SIGNAL(finished()), &m_playbackThread, SLOT(quit()));

	m_playbackTimer->setInterval(m_seqStepTime);
	emit UpdateStatus("Playing " + m_seqFileName);
	emit UpdatePlaybackStatus(m_seqFileName, PlaybackStatus::Playing);
	m_playbackThread.start();
}

void SequencePlayer::StartMusicSeq()
{
	if(!QFile::exists(m_mediaFile))
	{
		m_logger->error("Unable to find media file: {}", m_mediaFile.toStdString());
		emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
		return;
	}
	
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	//m_mediaPlayer->updateClock(200);
	m_mediaPlayer->setSource(QUrl::fromLocalFile(m_mediaFile));
#else
	m_mediaPlayer->setNotifyInterval(m_seqStepTime);
	m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_mediaFile));
#endif
	//auto test = m_mediaPlayer->mediaStatus();
	int count{100};
	while (m_mediaPlayer->mediaStatus() == QMediaPlayer::LoadingMedia && count > 0)
	{
		QCoreApplication::processEvents();
		QThread::msleep(10);
		--count;
	}
	//
	if (m_mediaPlayer->mediaStatus() != QMediaPlayer::LoadedMedia)
	{
		m_logger->error("Unable to Load media file{}: {}", static_cast<int>(m_mediaPlayer->mediaStatus()), m_mediaFile.toStdString());
		emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
		return;
	}
	//connect(&player, &QMediaPlayer::mediaStatusChanged,
	//	this, [&](QMediaPlayer::MediaStatus status) {
	//		if (status == QMediaPlayer::LoadedMedia) playClicked();
	//	});
	//m_mediaPlayer->setNotifyInterval(m_seqStepTime);
	emit UpdateStatus("Playing " + m_seqFileName);
	emit UpdatePlaybackStatus(m_seqFileName, PlaybackStatus::Playing);

#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	m_playbackTimer->setInterval(m_seqStepTime/2);
	m_playbackThread.start();
#endif
	m_mediaPlayer->play();
}

void SequencePlayer::MediaStatusChanged(QMediaPlayer::MediaStatus status)
{
	//if (status == QMediaPlayer::LoadedMedia) 
	//{
	//	emit UpdateStatus("Playing " + m_seqFileName);
	//	emit UpdatePlaybackStatus(m_seqFileName, PlaybackStatus::Playing);
	//	m_mediaPlayer->play();
	//}

	if (status == QMediaPlayer::InvalidMedia)
	{
		emit UpdateStatus("Faild to Open " + m_mediaFile);
		emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
	}
	
	m_logger->error("Media Status: {}", static_cast<int>(status));

}

#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
void SequencePlayer::MediaPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{

}
#else
void SequencePlayer::MediaStateChanged(QMediaPlayer::State state)
{

}
#endif

void SequencePlayer::SetMultisync(bool enabled)
{
	m_syncManager->SetEnabled(enabled);
}