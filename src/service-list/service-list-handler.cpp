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
#include <QPainter>
#include "radio.h"
#include "service-list-handler.h"

void CustomItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
  const QAbstractItemModel * pModel = index.model();
  const QModelIndex modIdxChannel = pModel->index(index.row(), ServiceDB::CI_Channel);
  const QModelIndex modIdxService = pModel->index(index.row(), ServiceDB::CI_Service);
  const bool isLiveChannel = (pModel->data(modIdxChannel).toString() == mCurChannel);
  const bool isLiveService = (pModel->data(modIdxService).toString() == mCurService);

  // see: https://colorpicker.me
  painter->fillRect(option.rect, (isLiveChannel ? (isLiveService ? 0x864e1a : 0x28005a) : 0x3c003c)); // background color

  if (index.column() == ServiceDB::CI_Fav)
  {
    const QPixmap & curPM = (index.data(Qt::DisplayRole).toBool() ? mStarFilledPixmap : mStarEmptyPixmap);

    // Calculate the position to center the pixmap within the item rectangle
    const int x = option.rect.x() + (option.rect.width()  - 16) / 2; // 16 = width of icon
    const int y = option.rect.y() + (option.rect.height() - 16) / 2; // 16 = height of icon

    painter->drawPixmap(x, y, curPM);
    return;
  }

  QStyledItemDelegate::paint(painter, option, index);
}

bool CustomItemDelegate::editorEvent(QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index)
{
  if (event->type() == QEvent::MouseButtonPress)
  {
    const QString channel = model->data(model->index(index.row(), ServiceDB::CI_Channel)).toString();
    const QString service = model->data(model->index(index.row(), ServiceDB::CI_Service)).toString();
    const bool    isFav   = model->data(model->index(index.row(), ServiceDB::CI_Fav)).toBool();

    emit signal_selection_changed(channel, service, isFav);

    //qDebug() << "Mouse click on cell: " << channel << ", " << service << ", " << isFav << " at " << index.row() << ", " << index.column();

    return true; // Indicate that the event is handled
  }

  return QStyledItemDelegate::editorEvent(event, model, option, index);
}


ServiceListHandler::ServiceListHandler(const QString & iDbFileName, QTableView * const ipSL) :
  mpTableView(ipSL),
  mServiceDB(iDbFileName)
{
  mpTableView->setItemDelegate(&mCustomItemDelegate);
  mpTableView->setSelectionMode(QAbstractItemView::NoSelection);
  mpTableView->verticalHeader()->hide(); // hide first column
  mpTableView->verticalHeader()->setDefaultSectionSize(0); // Use minimum possible size (seems work so)

  mServiceDB.open_db(); // program will exit if table could not be opened
  //mServiceDB.delete_table();
  mServiceDB.create_table();
  _fill_table_view_from_db();

  connect(mpTableView->horizontalHeader(), &QHeaderView::sectionClicked, this, &ServiceListHandler::_slot_header_clicked);
  connect(&mCustomItemDelegate, &CustomItemDelegate::signal_selection_changed, this, &ServiceListHandler::_slot_selection_changed);
}

void ServiceListHandler::add_entry(const QString & iChannel, const QString & iService)
{
  if (mServiceDB.add_entry(iChannel, iService)) // true if new entry was added
  {
    _fill_table_view_from_db();
    _jump_to_list_entry_and_emit_fav_status();
  }
}

void ServiceListHandler::delete_table(const bool iDeleteFavorites)
{
  mServiceDB.delete_table(iDeleteFavorites);
}

void ServiceListHandler::create_new_table()
{
  mServiceDB.create_table();
  _fill_table_view_from_db();
}

void ServiceListHandler::set_selector(const QString & iChannel, const QString & iService)
{
  if (iChannel.isEmpty() || iService.isEmpty())
  {
    return;
  }

  if (mChannelLast == iChannel && mServiceLast == iService)
  {
    return;
  }

  mChannelLast = iChannel;
  mServiceLast = iService;
  mCustomItemDelegate.set_current_service(mChannelLast, mServiceLast);

  _jump_to_list_entry_and_emit_fav_status();
}

void ServiceListHandler::set_favorite(const bool iIsFavorite)
{
  mServiceDB.set_favorite(mChannelLast, mServiceLast, iIsFavorite);
  _fill_table_view_from_db();
  _jump_to_list_entry_and_emit_fav_status();
}

void ServiceListHandler::restore_favorites()
{
  mServiceDB.retrieve_favorites_from_backup_table();
  _fill_table_view_from_db();
  _jump_to_list_entry_and_emit_fav_status();
}

void ServiceListHandler::_fill_table_view_from_db()
{
  mpTableView->setModel(mServiceDB.create_model());
  mpTableView->resizeColumnsToContents();
}

void ServiceListHandler::_jump_to_list_entry_and_emit_fav_status()
{
  if (mChannelLast.isEmpty() || mServiceLast.isEmpty())
  {
    return;
  }

  //qDebug() << "ServiceListHandler: Channel: " << mChannelLast << ", Service: " << mServiceLast;

  const QAbstractItemModel * const pModel = mpTableView->model();
  assert(pModel != nullptr);

  for (int rowIdx = 0; rowIdx < pModel->rowCount(); ++rowIdx)
  {
    QModelIndex modIdxChannel = pModel->index(rowIdx, ServiceDB::CI_Channel);
    QModelIndex modIdxService = pModel->index(rowIdx, ServiceDB::CI_Service);

    if (pModel->data(modIdxService).toString() == mServiceLast &&
        pModel->data(modIdxChannel).toString() == mChannelLast)
    {
      mpTableView->scrollTo(modIdxChannel, QAbstractItemView::EnsureVisible);
      mpTableView->update();

      const bool isFav = pModel->data(pModel->index(rowIdx, ServiceDB::CI_Fav)).toBool();
      emit signal_favorite_status(isFav);
      break;
    }
  }
}

void ServiceListHandler::_slot_selection_changed(const QString & iChannel, const QString & iService, const bool iIsFav)
{
  mChannelLast = iChannel;
  mServiceLast = iService;
  mCustomItemDelegate.set_current_service(mChannelLast, mServiceLast);
  mpTableView->update();
  
  emit signal_selection_changed(mChannelLast, mServiceLast);  // triggers an service change in radio.h
  emit signal_favorite_status(iIsFav); // this emit speeds up the FavButton setting, the other one is more important
}

void ServiceListHandler::_slot_header_clicked(int iIndex)
{
  mServiceDB.sort_column(static_cast<ServiceDB::EColIdx>(iIndex));
  _fill_table_view_from_db();
  _jump_to_list_entry_and_emit_fav_status();
}


