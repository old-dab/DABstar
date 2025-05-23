/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2013 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-TAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef  FIB_DECODER_H
#define  FIB_DECODER_H

//
#include  <cstdint>
#include  <cstdio>
#include  <QObject>
#include  <QByteArray>
#include  "msc-handler.h"
#include  <QMutex>
#include  "dab-config.h"

class DabRadio;
class EnsembleDescriptor;
class DabConfig;
class Cluster;

class FibDecoder : public QObject
{
Q_OBJECT
public:
  explicit FibDecoder(DabRadio *);
  ~FibDecoder();

  void clearEnsemble();
  void connect_channel();
  void disconnect_channel();
  bool sync_reached();
  void get_data_for_audio_service(const QString &, Audiodata *);
  void get_data_for_packet_service(const QString &, Packetdata *, int16_t);
  int get_sub_channel_id(const QString &, uint32_t);
  std::vector<SServiceId> get_services();

  QString find_service(uint32_t, int);
  void get_parameters(const QString &, uint32_t *, int *);
  uint8_t get_ecc();
  uint16_t get_country_name();
  uint8_t get_countryId();
  int32_t get_ensembleId();
  QString get_ensemble_name();
  void get_channel_info(ChannelData *, int);
  int32_t get_cif_count();
  void get_cif_count(int16_t *, int16_t *);
  uint32_t get_julian_date();
  void set_epg_data(uint32_t, int32_t, const QString &, const QString &);
  std::vector<SEpgElement> get_timeTable(uint32_t);
  std::vector<SEpgElement> get_timeTable(const QString &);
  bool has_time_table(uint32_t SId);
  std::vector<SEpgElement> find_epg_data(uint32_t);
  QStringList basic_print();
  int scan_width();

protected:
  void process_FIB(const uint8_t *, uint16_t);

private:
  std::vector<SServiceId> insert_sorted(const std::vector<SServiceId> & l, SServiceId n);
  DabRadio * myRadioInterface = nullptr;
  DabConfig * currentConfig = nullptr;
  DabConfig * nextConfig = nullptr;
  EnsembleDescriptor * ensemble = nullptr;
  std::array<int32_t, 8> dateTime{};
  QMutex fibLocker;
  int32_t CIFcount = 0;
  int16_t CIFcount_hi = 0;
  int16_t CIFcount_lo = 0;
  uint32_t mjd = 0;      // julianDate

  void process_FIG0(const uint8_t *);
  void process_FIG1(const uint8_t *);
  void FIG0Extension0(const uint8_t *);
  void FIG0Extension1(const uint8_t *);
  void FIG0Extension2(const uint8_t *);
  void FIG0Extension3(const uint8_t *);
  void FIG0Extension5(const uint8_t *);
  void FIG0Extension7(const uint8_t *);
  void FIG0Extension8(const uint8_t *);
  void FIG0Extension9(const uint8_t *);
  void FIG0Extension10(const uint8_t *);
  void FIG0Extension13(const uint8_t *);
  void FIG0Extension14(const uint8_t *);
  void FIG0Extension17(const uint8_t *);
  void FIG0Extension18(const uint8_t *);
  void FIG0Extension19(const uint8_t *);
  void FIG0Extension21(const uint8_t *);

  int16_t HandleFIG0Extension1(const uint8_t *, int16_t, uint8_t, uint8_t, uint8_t);
  int16_t HandleFIG0Extension2(const uint8_t *, int16_t, uint8_t, uint8_t, uint8_t);
  int16_t HandleFIG0Extension3(const uint8_t *, int16_t, uint8_t, uint8_t, uint8_t);
  int16_t HandleFIG0Extension5(const uint8_t *, uint8_t, uint8_t, uint8_t, int16_t);
  int16_t HandleFIG0Extension8(const uint8_t *, int16_t, uint8_t, uint8_t, uint8_t);
  int16_t HandleFIG0Extension13(const uint8_t *, int16_t, uint8_t, uint8_t, uint8_t);
  int16_t HandleFIG0Extension21(const uint8_t *, uint8_t, uint8_t, uint8_t, int16_t);

  void FIG1Extension0(const uint8_t *);
  void FIG1Extension1(const uint8_t *);
  void FIG1Extension4(const uint8_t *);
  void FIG1Extension5(const uint8_t *);
  void FIG1Extension6(const uint8_t *);

  int findService(const QString &);
  int find_service_index_from_SId(uint32_t);
  void cleanupServiceList();
  void createService(QString name, uint32_t SId, int SCIds);
  int findServiceComponent(DabConfig *, int16_t);
  int findComponent(DabConfig * db, uint32_t SId, int16_t subChId);
  int findServiceComponent(DabConfig *, uint32_t, uint8_t);
  void bind_audioService(DabConfig *, int8_t, uint32_t, int16_t, int16_t, int16_t, int16_t);
  void bind_packetService(DabConfig *, int8_t, uint32_t, int16_t, int16_t, int16_t, int16_t);

  QString announcements(uint16_t);

  void setCluster(DabConfig *, int, int16_t, uint16_t);
  Cluster * getCluster(DabConfig *, int16_t);

  QString serviceName(int index);
  QString serviceIdOf(int index);
  QString subChannelOf(int index);
  QString startAddressOf(int index);
  QString lengthOf(int index);
  QString protLevelOf(int index);
  QString codeRateOf(int index);
  QString bitRateOf(int index);
  QString dabType(int index);
  QString languageOf(int index);
  QString programTypeOf(int index);
  QString fmFreqOf(int index);
  QString appTypeOf(int index);
  QString FEC_scheme(int index);
  QString packetAddress(int index);
  QString DSCTy(int index);

  QString audioHeader();
  QString packetHeader();
  QString audioData(int index);
  QString packetData(int index);

signals:
  void signal_add_to_ensemble(const QString &, uint32_t);
  void signal_name_of_ensemble(int, const QString &);
  void signal_clock_time(int, int, int, int, int, int, int, int, int);
  void signal_change_in_configuration();
  void signal_start_announcement(const QString &, int);
  void signal_stop_announcement(const QString &, int);
  void signal_nr_services(int);
};

#endif

