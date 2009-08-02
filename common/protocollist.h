#include <QLinkedList>

class AbstractProtocol;

class ProtocolList : public QLinkedList<AbstractProtocol*>
{
public:
	void destroy();
};
