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

#pragma once

#include <QComboBox>
#include <QStringList>
#include <memory>
#include <tinia/model/ExposedModel.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/StateSchemaListener.hpp>

namespace tinia {
namespace qtcontroller {
namespace impl {

class ComboBox : public QComboBox, public model::StateListener,
         public model::StateSchemaListener
{
    Q_OBJECT
public:
    explicit ComboBox(std::string key, boost::shared_ptr<model::ExposedModel> model,
                      QWidget *parent = 0);
   ~ComboBox();

   void stateElementModified(model::StateElement *stateElement);
   void stateSchemaElementAdded(model::StateSchemaElement *stateSchemaElement);
   void stateSchemaElementModified(model::StateSchemaElement *stateSchemaElement);
   void stateSchemaElementRemoved(model::StateSchemaElement *stateSchemaElement);

signals:
   void newRestrictionsFromExposedModel();
   void setStateFromExposedModel(int);
   void clearFromExposedModel();
   void addItemsFromExposedModel(const QStringList& list);

public slots:
   void updateRestrictions();
private slots:
   void activatedChanged(QString value);
private:
   QStringList m_options;
   boost::shared_ptr<model::ExposedModel> m_model;
   std::string m_key;
};

}
} // namespace qtcontroller
} // namespace tinia


