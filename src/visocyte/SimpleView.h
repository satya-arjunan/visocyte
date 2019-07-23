/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SimpleView.h
  Language:  C++

  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef SimpleView_H
#define SimpleView_H

#include <random>
#include <map>
#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.
#include <QMainWindow>
#include <QKeyEvent>
#include <QAction>
#include <QToolButton>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkTable.h>
#include <vtkDelimitedTextReader.h>
#include <vtkDataObjectToTable.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include "vtkGenericOpenGLRenderWindow.h"

class Ui_SimpleView;
class vtkQtTableView;

class SimpleView : public QMainWindow
{
  Q_OBJECT
public:
  SimpleView();
  ~SimpleView() override;

public slots:
  virtual void open_file();
  virtual void exit();
  virtual void progress_slider_value_changed(int);
  virtual void init_points();
  virtual void init_colors();
  virtual void insert_color(int id, int index);
  virtual void update_points();
  virtual void init_random_points();
  virtual void update_random_points();
  virtual void inc_dec_frame();
  virtual void timeout_add();
  virtual void timeout_remove();
  virtual void on_timeout();
  virtual void set_frame(int value);
  virtual void set_progress_frame(int current_frame);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void play();
  virtual void pause();
  virtual void play_or_pause();

protected:
  void write_png();
  void step();
  void read_file(std::string input_file_name);
  void initialize();
  void reset();
  void set_unset_record();
  void write_time();
  void record_frame();

protected slots:

private:
  bool is_forward_;
  bool is_playing_;
  bool is_active_timeout_;
  bool is_initialized_;
  bool is_record_ = false;
  unsigned image_cnt_ = 0;
  double timer_interval_;
  double frame_interval_ = 20.63;
  int slider_value_;
  int id_column_;
  std::random_device rd_;
  std::mt19937_64 rng_;
  std::uniform_real_distribution<> uni_;
  vtkTable* table_;
  vtkSmartPointer<vtkQtTableView> tableview_;
  Ui_SimpleView *ui_;
  int timer_id_;
  int current_frame_;
  std::vector<int> frames_;
  std::vector<int> ids_;
  std::map<int, int> ids_map_;
  vtkSmartPointer<vtkPoints> points_;
  vtkSmartPointer<vtkPolyData> polydata_;
  vtkSmartPointer<vtkDelimitedTextReader> reader_;
  vtkSmartPointer<vtkUnsignedCharArray> colors_;
  vtkSmartPointer<vtkLookupTable> color_table_;
  vtkSmartPointer<vtkRenderer> renderer_;
  vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter_;
  vtkSmartPointer<vtkPolyDataMapper> mapper_;
  vtkSmartPointer<vtkActor> actor_;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> render_window_;
  vtkSmartPointer<vtkDataObjectToTable> to_table_;
  vtkSmartPointer<vtkTextActor> time_text_;
  QTimer* timer_;
  std::string filename_;
  QAction* play_action_;
  QAction* pause_action_;
  QAction* open_action_;
  QToolButton* play_button_;
  QToolButton* open_button_;
};

#endif // SimpleView_H
