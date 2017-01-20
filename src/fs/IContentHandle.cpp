#include "IContentHandle.h"
#include "INode.h"

void IContentHandle::fillAttr(struct stat &statbuf)
{
	getMeta()->fillAttr(statbuf);
}


bool IContentHandle::useDirectIO()
{
	return false;
}
