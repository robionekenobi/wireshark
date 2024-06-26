/** @file
 *
 * Copyright (c) 2005-2014 Informatica Corporation. All Rights Reserved.
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LBM_LBTRU_TRANSPORT_DIALOG_H
#define LBM_LBTRU_TRANSPORT_DIALOG_H

#include <config.h>

#include "cfile.h"
#include <epan/packet_info.h>
#include <epan/tap.h>
#include <QDialog>

class QHeaderView;
class QMenu;
class QTreeWidgetItem;

namespace Ui
{
    class LBMLBTRUTransportDialog;
}

class LBMLBTRUTransportDialogInfo;
class LBMLBTRUSourceTransportEntry;
class LBMLBTRUReceiverTransportEntry;

class LBMLBTRUTransportDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit LBMLBTRUTransportDialog(QWidget * parent = 0, capture_file * cfile = NULL);
        Ui::LBMLBTRUTransportDialog * getUI(void)
        {
            return (m_ui);
        }
    public slots:
        void setCaptureFile(capture_file * cfile);

    signals:
        void goToPacket(int packet_num);

    private:
        Ui::LBMLBTRUTransportDialog * m_ui;
        LBMLBTRUTransportDialogInfo * m_dialog_info;
        capture_file * m_capture_file;
        LBMLBTRUSourceTransportEntry * m_current_source_transport;
        LBMLBTRUReceiverTransportEntry * m_current_receiver_transport;
        QMenu * m_source_context_menu;
        QHeaderView * m_source_header;
        QMenu * m_receiver_context_menu;
        QHeaderView * m_receiver_header;

        virtual ~LBMLBTRUTransportDialog(void);
        void resetSources(void);
        void resetReceivers(void);
        void resetSourcesDetail(void);
        void resetReceiversDetail(void);
        void fillTree(void);
        static void resetTap(void * tap_data);
        static tap_packet_status tapPacket(void * tap_data, packet_info * pinfo, epan_dissect_t * edt, const void * stream_info, tap_flags_t flags);
        static void drawTreeItems(void * tap_data);
        void loadSourceDataDetails(LBMLBTRUSourceTransportEntry * transport);
        void loadSourceRXDataDetails(LBMLBTRUSourceTransportEntry * transport);
        void loadSourceNCFDetails(LBMLBTRUSourceTransportEntry * transport);
        void loadSourceSMDetails(LBMLBTRUSourceTransportEntry * transport);
        void loadSourceRSTDetails(LBMLBTRUSourceTransportEntry * transport);
        void loadReceiverNAKDetails(LBMLBTRUReceiverTransportEntry * transport);
        void loadReceiverACKDetails(LBMLBTRUReceiverTransportEntry * transport);
        void loadReceiverCREQDetails(LBMLBTRUReceiverTransportEntry * transport);

    private slots:
        void on_applyFilterButton_clicked(void);

        void sourcesDetailCurrentChanged(int index);
        void sourcesItemClicked(QTreeWidgetItem * item, int column);
        void sourcesDetailItemDoubleClicked(QTreeWidgetItem * item, int column);
        void receiversDetailCurrentChanged(int index);
        void receiversItemClicked(QTreeWidgetItem * item, int column);
        void receiversDetailItemDoubleClicked(QTreeWidgetItem * item, int column);

        void custom_source_context_menuRequested(const QPoint & pos);
        void actionSourceDataFrames_triggered(bool checked);
        void actionSourceDataBytes_triggered(bool checked);
        void actionSourceDataFramesBytes_triggered(bool checked);
        void actionSourceDataRate_triggered(bool checked);
        void actionSourceRXDataFrames_triggered(bool checked);
        void actionSourceRXDataBytes_triggered(bool checked);
        void actionSourceRXDataFramesBytes_triggered(bool checked);
        void actionSourceRXDataRate_triggered(bool checked);
        void actionSourceNCFFrames_triggered(bool checked);
        void actionSourceNCFCount_triggered(bool checked);
        void actionSourceNCFBytes_triggered(bool checked);
        void actionSourceNCFFramesBytes_triggered(bool checked);
        void actionSourceNCFCountBytes_triggered(bool checked);
        void actionSourceNCFFramesCount_triggered(bool checked);
        void actionSourceNCFFramesCountBytes_triggered(bool checked);
        void actionSourceNCFRate_triggered(bool checked);
        void actionSourceSMFrames_triggered(bool checked);
        void actionSourceSMBytes_triggered(bool checked);
        void actionSourceSMFramesBytes_triggered(bool checked);
        void actionSourceSMRate_triggered(bool checked);
        void actionSourceAutoResizeColumns_triggered(void);
        void custom_receiver_context_menuRequested(const QPoint & pos);
        void actionReceiverNAKFrames_triggered(bool checked);
        void actionReceiverNAKCount_triggered(bool checked);
        void actionReceiverNAKBytes_triggered(bool checked);
        void actionReceiverNAKFramesCount_triggered(bool checked);
        void actionReceiverNAKCountBytes_triggered(bool checked);
        void actionReceiverNAKFramesBytes_triggered(bool checked);
        void actionReceiverNAKFramesCountBytes_triggered(bool checked);
        void actionReceiverNAKRate_triggered(bool checked);
        void actionReceiverACKFrames_triggered(bool checked);
        void actionReceiverACKBytes_triggered(bool checked);
        void actionReceiverACKFramesBytes_triggered(bool checked);
        void actionReceiverACKRate_triggered(bool checked);
        void actionReceiverCREQFrames_triggered(bool checked);
        void actionReceiverCREQBytes_triggered(bool checked);
        void actionReceiverCREQFramesBytes_triggered(bool checked);
        void actionReceiverCREQRate_triggered(bool checked);
        void actionReceiverAutoResizeColumns_triggered(void);
};

#endif
