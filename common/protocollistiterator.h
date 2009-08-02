#include <QMutableLinkedListIterator>

class AbstractProtocol;
class ProtocolList;

class ProtocolListIterator 
{
private:
	QMutableLinkedListIterator<AbstractProtocol*> *_iter;

public:
	ProtocolListIterator(ProtocolList &list);
	~ProtocolListIterator();
	bool findNext(const AbstractProtocol* value) const;
	bool findPrevious(const AbstractProtocol* value);
	bool hasNext() const;
	bool hasPrevious() const;
	void insert(const AbstractProtocol* value);
	AbstractProtocol* next();
	AbstractProtocol* peekNext() const;
	AbstractProtocol* peekPrevious() const;
	AbstractProtocol* previous();
	void remove();
	void setValue(const AbstractProtocol* value) const;
	void toBack();
	void toFront();
	const AbstractProtocol* value() const;
	AbstractProtocol* value();
};
