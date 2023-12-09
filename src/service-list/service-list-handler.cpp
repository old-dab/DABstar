/*
 * Copyright (c) 2023 by Thomas Neder (https://github.com/tomneda)
 *
 * DABstar is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the License, or any later version.
 *
 * DABstar is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with DABstar. If not, write to the Free Software
 * Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <QTableView>
#include "radio.h"
#include "service-list-handler.h"

ServiceListHandler::ServiceListHandler(RadioInterface * ipRI, QTableView * const ipSL) :
  mpRadio(ipRI),
  mpTableView(ipSL),
  mServiceDB("/home/work/servicelist_v01.db")
{
  //mServiceDB.delete_table();
  mServiceDB.create_table();

  //connect(mpTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ServiceListHandler::_slot_selection_changed);
}

void ServiceListHandler::update()
{
  // hope this makes no problem if first time no model is attached to QTableView
  disconnect(mpTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ServiceListHandler::_slot_selection_changed);

  mpTableView->setModel(mServiceDB.create_model());
  mpTableView->resizeColumnsToContents();
  mpTableView->verticalHeader()->setDefaultSectionSize(0); // Use minimum possible size (seems work so but not the below)
  //mpTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  //mpTableView->verticalHeader()->setDefaultSectionSize(mpTableView->verticalHeader()->minimumSectionSize());
  mpTableView->setSelectionMode(QAbstractItemView::SingleSelection); // Allow only one row to be selected at a time
  mpTableView->setSelectionBehavior(QAbstractItemView::SelectRows);  // Select entire rows, not individual items
  mpTableView->verticalHeader()->hide();
  mpTableView->show();

  connect(mpTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ServiceListHandler::_slot_selection_changed);
}

void ServiceListHandler::_slot_selection_changed(const QItemSelection & selected, const QItemSelection & deselected)
{
  // Fetch the first index of the selected items
  QModelIndexList indexes = selected.indexes();

  if(!indexes.empty())
  {
    int row = indexes.first().row();
    QVector<QVariant> rowData;

    const QString curService = mpTableView->model()->index(row, 0).data().toString();
    const QString curChannel = mpTableView->model()->index(row, 1).data().toString();

    if (curChannel != mLastChannel)
    {
      emit signal_channel_changed(curChannel, curService);
    }
    else
    {
      emit signal_service_changed(curService);
    }

    mLastChannel = curChannel;
  }
}
