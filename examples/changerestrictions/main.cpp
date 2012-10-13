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

#include "RestrictionsJob.hpp"
#include "tinia/qtcontroller/QTController.hpp"
int main(int argc, char** argv)
{
    using namespace tinia::example;
    tinia::qtcontroller::QTController *qtController = new tinia::qtcontroller::QTController();
    RestrictionsJob *testJob = new RestrictionsJob();
    qtController->setJob(testJob);

    qtController->run(argc, argv);

}