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
#include <string>
#include <memory>
#include "tinia/policy/Policy.hpp"
namespace tinia {
namespace policyxml {
class ElementHandler
{
public:
   ElementHandler(std::shared_ptr<policy::Policy> policy);
   void updateElementFromString(const std::string& key,
                                const std::string& stringValue);
   void updateElementFromPTree(const std::string& key,
                               const policy::StringStringPTree& tree);

private:
   std::shared_ptr<policy::Policy> m_policy;
};
}
}