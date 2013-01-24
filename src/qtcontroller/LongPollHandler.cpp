#include "tinia/qtcontroller/moc/LongPollHandler.hpp"
#include "tinia/qtcontroller/impl/http_utils.hpp"
#include <QDate>

namespace tinia {
namespace qtcontroller {
namespace impl {

LongPollHandler::LongPollHandler(QTextStream& os,  const QString& request,
                                           std::shared_ptr<tinia::model::ExposedModel> model,
                                           QObject *parent) :
    QObject(parent), m_request(request),
    m_model(model), m_xmlHandler(model), m_textStream(os)
{
    m_model->addStateListener(this);
}

LongPollHandler::~LongPollHandler()
{
    m_model->removeStateListener(this);
}

void LongPollHandler::handle()
{
    unsigned int revision = 0;
    try {
        auto args = decodeGetParameters(m_request);
        auto params = parseGet<boost::tuple<unsigned int> >(decodeGetParameters(m_request), "revision");
        revision = params.get<0>();
    } catch(std::invalid_argument& e) {
        // Don't have to do anything;
    }

    if(!addExposedModelUpdate(m_textStream, revision)) {
        // Add http-timeout
        m_mutex.lock();
        m_waitCondition.wait(&m_mutex, 500000);
        m_mutex.unlock();
		if(!addExposedModelUpdate(m_textStream, revision)) {
			m_textStream << httpHeader("text/plain", 408);
		}
    }
}

void LongPollHandler::stateElementModified(model::StateElement *stateElement)
{
    m_mutex.lock();
    m_waitCondition.wakeAll();
    m_mutex.unlock();
}

bool LongPollHandler::addExposedModelUpdate(QTextStream &os, unsigned int revision)
{
    //if(num>4) return true;
    auto length = m_xmlHandler.getExposedModelUpdate(m_buffer, sizeof(m_buffer), revision);
    if(length > 0) {
        os << httpHeader("application/xml")<<"\r\n";
        os << QString(m_buffer)<< "\n";
        return true;
    }
    else {
        return false;
    }
}

} // namespace impl
} // namespace qtcontroller
} // namespace tinia
