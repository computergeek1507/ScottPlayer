#ifndef BASEOUTPUT_H
#define BASEOUTPUT_H

#include <QObject>
#include <QString>

#include <memory>

struct BaseOutput: public QObject
{
	Q_OBJECT
public:
	virtual void Open() const = 0;
	virtual void OutputFrame(uint8_t *data) const = 0;

	uint64_t StartChannel{0};
};

#endif