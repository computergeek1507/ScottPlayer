#ifndef BASEOUTPUT_H
#define BASEOUTPUT_H

#include <QObject>
#include <QString>

#include <memory>

struct BaseOutput: public QObject
{
	Q_OBJECT
public:
	virtual bool Open() = 0;
	virtual void OutputFrame(uint8_t *data) = 0;
	virtual void Close() = 0;

	uint64_t StartChannel{0};
	uint64_t Channels{0};
	QString IP;
	bool Enabled{true};
};

#endif