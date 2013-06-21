/**
 * @licence app begin@
 * Copyright (C) 2011-2012  BMW AG
 *
 * This file is part of GENIVI Project Dlt Viewer.
 *
 * Contributions are licensed to the GENIVI Alliance under one or more
 * Contribution License Agreements.
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \file treemodel.cpp
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#include <QtGui>
#include <qmessagebox.h>

#include "tablemodel.h"
#include "fieldnames.h"
#include "dltsettingsmanager.h"
#include "dltuiutils.h"

char buffer[DLT_VIEWER_LIST_BUFFER_SIZE];

 TableModel::TableModel(const QString & /*data*/, QObject *parent)
     : QAbstractTableModel(parent)
 {
     lastSearchIndex = -1;
 }

 TableModel::~TableModel()
 {

 }

 int TableModel::columnCount(const QModelIndex & /*parent*/) const
 {
     return DLT_VIEWER_COLUMN_COUNT;
 }

 QVariant TableModel::data(const QModelIndex &index, int role) const
 {
     QDltMsg msg;
     QByteArray buf;

     if (!index.isValid())
         return QVariant();

     if (index.row() >= qfile->sizeFilter() && index.row()<0)
         return QVariant();

     if (role == Qt::DisplayRole)
     {
         /* get the message with the selected item id */
         if(!qfile->getMsg(qfile->getMsgFilterPos(index.row()), msg))
         {
             if(index.column() == FieldNames::Index)
             {
                 return QString("%1").arg(qfile->getMsgFilterPos(index.row()));
             }
             else if(index.column() == FieldNames::Payload)
             {
                 return QString("!!CORRUPTED MESSAGE!!");
             }
             return QVariant();
         }
         if((DltSettingsManager::getInstance()->value("startup/pluginsEnabled", true).toBool()))
         {
             for(int num = 0; num < project->plugin->topLevelItemCount (); num++)
             {
                 PluginItem *item = (PluginItem*)project->plugin->topLevelItem(num);

                 if(item->getMode() != item->ModeDisable && item->plugindecoderinterface && item->plugindecoderinterface->isMsg(msg,0))
                 {
                     item->plugindecoderinterface->decodeMsg(msg,0);
                     break;
                 }
             }
         }

         switch(index.column())
         {
         case FieldNames::Index:
             /* display index */
             return QString("%1").arg(qfile->getMsgFilterPos(index.row()));
         case FieldNames::Time:
             if( project->settings->automaticTimeSettings == 0 )
                return QString("%1.%2").arg(msg.getGmTimeWithOffsetString(project->settings->utcOffset,project->settings->dst)).arg(msg.getMicroseconds(),6,10,QLatin1Char('0'));
             else
                return QString("%1.%2").arg(msg.getTimeString()).arg(msg.getMicroseconds(),6,10,QLatin1Char('0'));
         case FieldNames::TimeStamp:
             return QString("%1.%2").arg(msg.getTimestamp()/10000).arg(msg.getTimestamp()%10000,4,10,QLatin1Char('0'));
         case FieldNames::Counter:
             return QString("%1").arg(msg.getMessageCounter());
         case FieldNames::EcuId:
             return msg.getEcuid();
         case FieldNames::AppId:
             switch(project->settings->showApIdDesc){
             case 0:
                 return msg.getApid();
                 break;
             case 1:
                   for(int num = 0; num < project->ecu->topLevelItemCount (); num++)
                    {
                     EcuItem *ecuitem = (EcuItem*)project->ecu->topLevelItem(num);
                     for(int numapp = 0; numapp < ecuitem->childCount(); numapp++)
                     {
                         ApplicationItem * appitem = (ApplicationItem *) ecuitem->child(numapp);
                         if(appitem->id == msg.getApid() && !appitem->description.isEmpty())
                         {
                            return appitem->description;
                         }
                     }
                    }
                   return QString("Apid: %1 (No description)").arg(msg.getApid());
                 break;
              default:
                 return msg.getApid();
             }
         case FieldNames::ContextId:
             switch(project->settings->showCtIdDesc){
             case 0:
                 return msg.getCtid();
                 break;
             case 1:

                   for(int num = 0; num < project->ecu->topLevelItemCount (); num++)
                    {
                     EcuItem *ecuitem = (EcuItem*)project->ecu->topLevelItem(num);
                     for(int numapp = 0; numapp < ecuitem->childCount(); numapp++)
                     {
                         ApplicationItem * appitem = (ApplicationItem *) ecuitem->child(numapp);
                         for(int numcontext = 0; numcontext < appitem->childCount(); numcontext++)
                         {
                             ContextItem * conitem = (ContextItem *) appitem->child(numcontext);

                             if(appitem->id == msg.getApid() && conitem->id == msg.getCtid()
                                     && !conitem->description.isEmpty())
                             {
                                return conitem->description;
                             }
                         }
                     }
                    }
                   return  QString("Ctid: %1 (No description)").arg(msg.getCtid());
                 break;
              default:
                 return msg.getCtid();
             }
         case FieldNames::Type:
             return msg.getTypeString();
         case FieldNames::Subtype:
             return msg.getSubtypeString();
         case FieldNames::Mode:
             return msg.getModeString();
         case FieldNames::ArgCount:
             return QString("%1").arg(msg.getNumberOfArguments());
         case FieldNames::Payload:
             /* display payload */
             return msg.toStringPayload();
         }
     }

     if ( role == Qt::ForegroundRole ) {
         int currentIdx = qfile->getMsgFilterPos(index.row());
         qfile->getMsg(currentIdx, msg);

         // Color the last search row         
         if(lastSearchIndex != -1 && currentIdx == qfile->getMsgFilterPos(lastSearchIndex))
         {
             return QVariant(QBrush(DltUiUtils::optimalTextColor(searchBackgroundColor())));

         }
         else if (qfile->checkMarker(msg).isValid())
         {
           QColor color = qfile->checkMarker(msg);

              //return QVariant(QBrush(color));
              return QVariant(QBrush(DltUiUtils::optimalTextColor(color)));
         }
         else if(project->settings->autoMarkFatalError && !qfile->checkMarker(msg).isValid() && ( msg.getSubtypeString() == "error" || msg.getSubtypeString() == "fatal")  ){
            return QVariant(QBrush(QColor(255,255,255)));
         } else {
            return QVariant(QBrush(QColor(0,0,0)));
         }
     }

     if ( role == Qt::BackgroundRole ) {
         int currentIdx = qfile->getMsgFilterPos(index.row());
         qfile->getMsg(currentIdx, msg);

         if(lastSearchIndex != -1 && currentIdx == qfile->getMsgFilterPos(lastSearchIndex))
         {
             return QVariant(QBrush(searchBackgroundColor()));
         }

         if((DltSettingsManager::getInstance()->value("startup/pluginsEnabled", true).toBool()))
         {
             for(int num = 0; num < project->plugin->topLevelItemCount (); num++)
             {
                 PluginItem *item = (PluginItem*)project->plugin->topLevelItem(num);

                 if(item->getMode() != item->ModeDisable && item->plugindecoderinterface && item->plugindecoderinterface->isMsg(msg,0))
                 {
                     item->plugindecoderinterface->decodeMsg(msg,0);
                     break;
                 }
             }
         }

         QColor color = qfile->checkMarker(msg);
         if(color.isValid())
         {
            return QVariant(QBrush(color));
         }
         else
         {
             if(project->settings->autoMarkFatalError && ( msg.getSubtypeString() == "error" || msg.getSubtypeString() == "fatal") ){
                return QVariant(QBrush(QColor(255,0,0)));
             }
             if(project->settings->autoMarkWarn && msg.getSubtypeString() == "warn"){
                return QVariant(QBrush(QColor(255,255,0)));
             }

             return QVariant(QBrush(QColor(255,255,255)));
         }
     }

     if ( role == Qt::TextAlignmentRole ) {
        switch(index.column())
        {
            case FieldNames::Index:
                return QVariant(Qt::AlignRight  | Qt::AlignVCenter);
            case FieldNames::Time:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::TimeStamp:
                return QVariant(Qt::AlignRight  | Qt::AlignVCenter);
            case FieldNames::Counter:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::EcuId:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::AppId:
                switch(project->settings->showApIdDesc){
                case 0:
                    return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
                    break;
                case 1:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                default:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                }
            case FieldNames::ContextId:
                switch(project->settings->showCtIdDesc){
                case 0:
                    return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
                    break;
                case 1:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                default:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                }
            case FieldNames::Type:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::Subtype:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::Mode:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::ArgCount:
                return QVariant(Qt::AlignRight  | Qt::AlignVCenter);
            case FieldNames::Payload:
                return QVariant(Qt::AlignLeft   | Qt::AlignVCenter);
        }
    }

     return QVariant();
 }

QVariant TableModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        return FieldNames::getName((FieldNames::Fields)section, project->settings);
    }

    return QVariant();
}

 int TableModel::rowCount(const QModelIndex & /*parent*/) const
 {
    return qfile->sizeFilter();
 }

 void TableModel::modelChanged()
 {
     QModelIndex lIndex = index(0, 1);
     QModelIndex lLeft = index(qfile->sizeFilter()-1, 0);
     QModelIndex lRight = index(qfile->sizeFilter()-1, columnCount() - 1);
     emit(layoutChanged());
 }




QColor TableModel::searchBackgroundColor() const
{
    QString color = DltSettingsManager::getInstance()->value("other/searchResultColor", QString("#00AAFF")).toString();
    QColor hlColor(color);
    return hlColor;
}
