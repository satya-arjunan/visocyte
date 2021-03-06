/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// QT includes
#include <QtWidgets/QApplication>
#include <QtGui/QSurfaceFormat>

#include "QVTKOpenGLWidget.h"
#include "Viewer.hpp"

extern int qInitResources_icons();


int main( int argc, char** argv )
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  auto defaultFormat = QVTKOpenGLWidget::defaultFormat();
  defaultFormat.setSamples(0);
  QSurfaceFormat::setDefaultFormat(defaultFormat);

  // QT Stuff
  QApplication app( argc, argv );

  QApplication::setStyle("Material");

  qInitResources_icons();

  Viewer viewer;
  viewer.show();

  return app.exec();
}
