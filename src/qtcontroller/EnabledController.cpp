/* Copyright STIFTELSEN SINTEF 2012
 * 
 * This file is part of the Tinia Framework.
 * 
 * The Tinia Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * The Tinia Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with the Tinia Framework.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "tinia/qtcontroller/moc/EnabledController.hpp"

namespace tinia {
namespace qtcontroller {
namespace impl {

EnabledController::EnabledController( QWidget*                               widget,
                                      boost::shared_ptr<model::ExposedModel>  model,
                                      const std::string&                     key,
                                      const bool                             inverted )
    : QObject( widget ),
      m_model( model ),
      m_key( key ),
      m_inverted( inverted )
{
    // We set the widget as the parent, and qt should delete this object when
    // widget is deleted. Otherwise, we could do connect the widget's destroyed
    // signal to this object deleteLater slot (but this shouldn't be necessary)
    connect( this, SIGNAL(setWidgetEnabled(bool)), widget, SLOT(setEnabled(bool)) );

    m_model->addStateListener( m_key, this );

    bool value;
    m_model->getElementValue( m_key, value );

    bool enabled = (value && !m_inverted ) || (!value && m_inverted );
    widget->setEnabled( enabled );
}

EnabledController::~EnabledController()
{
    m_model->removeStateListener( m_key, this );
}

void
EnabledController::stateElementModified(model::StateElement *stateElement)
{
    bool value;
    stateElement->getValue( value );

    bool enabled = (value && !m_inverted ) || (!value && m_inverted );
    emit setWidgetEnabled( enabled );
}

}
} // of namespace qtcontroller
} // of namespace tinia
