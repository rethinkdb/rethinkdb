// Boost.Polygon library voronoi_visualizer.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <iostream>
#include <vector>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QtGui>

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
using namespace boost::polygon;

#include "voronoi_visual_utils.hpp"

class GLWidget : public QGLWidget {
  Q_OBJECT

 public:
  explicit GLWidget(QMainWindow* parent = NULL) :
      QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
      primary_edges_only_(false),
      internal_edges_only_(false) {
    startTimer(40);
  }

  QSize sizeHint() const {
    return QSize(600, 600);
  }

  void build(const QString& file_path) {
    // Clear all containers.
    clear();

    // Read data.
    read_data(file_path);

    // No data, don't proceed.
    if (!brect_initialized_) {
      return;
    }

    // Construct bounding rectangle.
    construct_brect();

    // Construct voronoi diagram.
    construct_voronoi(
        point_data_.begin(), point_data_.end(),
        segment_data_.begin(), segment_data_.end(),
        &vd_);

    // Color exterior edges.
    for (const_edge_iterator it = vd_.edges().begin();
         it != vd_.edges().end(); ++it) {
      if (!it->is_finite()) {
        color_exterior(&(*it));
      }
    }

    // Update view port.
    update_view_port();
  }

  void show_primary_edges_only() {
    primary_edges_only_ ^= true;
  }

  void show_internal_edges_only() {
    internal_edges_only_ ^= true;
  }

 protected:
  void initializeGL() {
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
  }

  void paintGL() {
    qglClearColor(QColor::fromRgb(255, 255, 255));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_points();
    draw_segments();
    draw_vertices();
    draw_edges();
  }

  void resizeGL(int width, int height) {
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
  }

  void timerEvent(QTimerEvent* e) {
    update();
  }

 private:
  typedef double coordinate_type;
  typedef point_data<coordinate_type> point_type;
  typedef segment_data<coordinate_type> segment_type;
  typedef rectangle_data<coordinate_type> rect_type;
  typedef voronoi_builder<int> VB;
  typedef voronoi_diagram<coordinate_type> VD;
  typedef VD::cell_type cell_type;
  typedef VD::cell_type::source_index_type source_index_type;
  typedef VD::cell_type::source_category_type source_category_type;
  typedef VD::edge_type edge_type;
  typedef VD::cell_container_type cell_container_type;
  typedef VD::cell_container_type vertex_container_type;
  typedef VD::edge_container_type edge_container_type;
  typedef VD::const_cell_iterator const_cell_iterator;
  typedef VD::const_vertex_iterator const_vertex_iterator;
  typedef VD::const_edge_iterator const_edge_iterator;

  static const std::size_t EXTERNAL_COLOR = 1;

  void clear() {
    brect_initialized_ = false;
    point_data_.clear();
    segment_data_.clear();
    vd_.clear();
  }

  void read_data(const QString& file_path) {
    QFile data(file_path);
    if (!data.open(QFile::ReadOnly)) {
      QMessageBox::warning(
          this, tr("Voronoi Visualizer"),
          tr("Disable to open file ") + file_path);
    }
    QTextStream in_stream(&data);
    std::size_t num_points, num_segments;
    int x1, y1, x2, y2;
    in_stream >> num_points;
    for (std::size_t i = 0; i < num_points; ++i) {
      in_stream >> x1 >> y1;
      point_type p(x1, y1);
      update_brect(p);
      point_data_.push_back(p);
    }
    in_stream >> num_segments;
    for (std::size_t i = 0; i < num_segments; ++i) {
      in_stream >> x1 >> y1 >> x2 >> y2;
      point_type lp(x1, y1);
      point_type hp(x2, y2);
      update_brect(lp);
      update_brect(hp);
      segment_data_.push_back(segment_type(lp, hp));
    }
    in_stream.flush();
  }

  void update_brect(const point_type& point) {
    if (brect_initialized_) {
      encompass(brect_, point);
    } else {
      set_points(brect_, point, point);
      brect_initialized_ = true;
    }
  }

  void construct_brect() {
    double side = (std::max)(xh(brect_) - xl(brect_), yh(brect_) - yl(brect_));
    center(shift_, brect_);
    set_points(brect_, shift_, shift_);
    bloat(brect_, side * 1.2);
  }

  void color_exterior(const VD::edge_type* edge) {
    if (edge->color() == EXTERNAL_COLOR) {
      return;
    }
    edge->color(EXTERNAL_COLOR);
    edge->twin()->color(EXTERNAL_COLOR);
    const VD::vertex_type* v = edge->vertex1();
    if (v == NULL || !edge->is_primary()) {
      return;
    }
    v->color(EXTERNAL_COLOR);
    const VD::edge_type* e = v->incident_edge();
    do {
      color_exterior(e);
      e = e->rot_next();
    } while (e != v->incident_edge());
  }

  void update_view_port() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    rect_type view_rect = brect_;
    deconvolve(view_rect, shift_);
    glOrtho(xl(view_rect), xh(view_rect),
            yl(view_rect), yh(view_rect),
            -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
  }

  void draw_points() {
    // Draw input points and endpoints of the input segments.
    glColor3f(0.0f, 0.5f, 1.0f);
    glPointSize(9);
    glBegin(GL_POINTS);
    for (std::size_t i = 0; i < point_data_.size(); ++i) {
      point_type point = point_data_[i];
      deconvolve(point, shift_);
      glVertex2f(point.x(), point.y());
    }
    for (std::size_t i = 0; i < segment_data_.size(); ++i) {
      point_type lp = low(segment_data_[i]);
      lp = deconvolve(lp, shift_);
      glVertex2f(lp.x(), lp.y());
      point_type hp = high(segment_data_[i]);
      hp = deconvolve(hp, shift_);
      glVertex2f(hp.x(), hp.y());
    }
    glEnd();
  }

  void draw_segments() {
    // Draw input segments.
    glColor3f(0.0f, 0.5f, 1.0f);
    glLineWidth(2.7f);
    glBegin(GL_LINES);
    for (std::size_t i = 0; i < segment_data_.size(); ++i) {
      point_type lp = low(segment_data_[i]);
      lp = deconvolve(lp, shift_);
      glVertex2f(lp.x(), lp.y());
      point_type hp = high(segment_data_[i]);
      hp = deconvolve(hp, shift_);
      glVertex2f(hp.x(), hp.y());
    }
    glEnd();
  }

  void draw_vertices() {
    // Draw voronoi vertices.
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(6);
    glBegin(GL_POINTS);
    for (const_vertex_iterator it = vd_.vertices().begin();
         it != vd_.vertices().end(); ++it) {
      if (internal_edges_only_ && (it->color() == EXTERNAL_COLOR)) {
        continue;
      }
      point_type vertex(it->x(), it->y());
      vertex = deconvolve(vertex, shift_);
      glVertex2f(vertex.x(), vertex.y());
    }
    glEnd();
  }
  void draw_edges() {
    // Draw voronoi edges.
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.7f);
    for (const_edge_iterator it = vd_.edges().begin();
         it != vd_.edges().end(); ++it) {
      if (primary_edges_only_ && !it->is_primary()) {
        continue;
      }
      if (internal_edges_only_ && (it->color() == EXTERNAL_COLOR)) {
        continue;
      }
      std::vector<point_type> samples;
      if (!it->is_finite()) {
        clip_infinite_edge(*it, &samples);
      } else {
        point_type vertex0(it->vertex0()->x(), it->vertex0()->y());
        samples.push_back(vertex0);
        point_type vertex1(it->vertex1()->x(), it->vertex1()->y());
        samples.push_back(vertex1);
        if (it->is_curved()) {
          sample_curved_edge(*it, &samples);
        }
      }
      glBegin(GL_LINE_STRIP);
      for (std::size_t i = 0; i < samples.size(); ++i) {
        point_type vertex = deconvolve(samples[i], shift_);
        glVertex2f(vertex.x(), vertex.y());
      }
      glEnd();
    }
  }

  void clip_infinite_edge(
      const edge_type& edge, std::vector<point_type>* clipped_edge) {
    const cell_type& cell1 = *edge.cell();
    const cell_type& cell2 = *edge.twin()->cell();
    point_type origin, direction;
    // Infinite edges could not be created by two segment sites.
    if (cell1.contains_point() && cell2.contains_point()) {
      point_type p1 = retrieve_point(cell1);
      point_type p2 = retrieve_point(cell2);
      origin.x((p1.x() + p2.x()) * 0.5);
      origin.y((p1.y() + p2.y()) * 0.5);
      direction.x(p1.y() - p2.y());
      direction.y(p2.x() - p1.x());
    } else {
      origin = cell1.contains_segment() ?
          retrieve_point(cell2) :
          retrieve_point(cell1);
      segment_type segment = cell1.contains_segment() ?
          retrieve_segment(cell1) :
          retrieve_segment(cell2);
      coordinate_type dx = high(segment).x() - low(segment).x();
      coordinate_type dy = high(segment).y() - low(segment).y();
      if ((low(segment) == origin) ^ cell1.contains_point()) {
        direction.x(dy);
        direction.y(-dx);
      } else {
        direction.x(-dy);
        direction.y(dx);
      }
    }
    coordinate_type side = xh(brect_) - xl(brect_);
    coordinate_type koef =
        side / (std::max)(fabs(direction.x()), fabs(direction.y()));
    if (edge.vertex0() == NULL) {
      clipped_edge->push_back(point_type(
          origin.x() - direction.x() * koef,
          origin.y() - direction.y() * koef));
    } else {
      clipped_edge->push_back(
          point_type(edge.vertex0()->x(), edge.vertex0()->y()));
    }
    if (edge.vertex1() == NULL) {
      clipped_edge->push_back(point_type(
          origin.x() + direction.x() * koef,
          origin.y() + direction.y() * koef));
    } else {
      clipped_edge->push_back(
          point_type(edge.vertex1()->x(), edge.vertex1()->y()));
    }
  }

  void sample_curved_edge(
      const edge_type& edge,
      std::vector<point_type>* sampled_edge) {
    coordinate_type max_dist = 1E-3 * (xh(brect_) - xl(brect_));
    point_type point = edge.cell()->contains_point() ?
        retrieve_point(*edge.cell()) :
        retrieve_point(*edge.twin()->cell());
    segment_type segment = edge.cell()->contains_point() ?
        retrieve_segment(*edge.twin()->cell()) :
        retrieve_segment(*edge.cell());
    voronoi_visual_utils<coordinate_type>::discretize(
        point, segment, max_dist, sampled_edge);
  }

  point_type retrieve_point(const cell_type& cell) {
    source_index_type index = cell.source_index();
    source_category_type category = cell.source_category();
    if (category == SOURCE_CATEGORY_SINGLE_POINT) {
      return point_data_[index];
    }
    index -= point_data_.size();
    if (category == SOURCE_CATEGORY_SEGMENT_START_POINT) {
      return low(segment_data_[index]);
    } else {
      return high(segment_data_[index]);
    }
  }

  segment_type retrieve_segment(const cell_type& cell) {
    source_index_type index = cell.source_index() - point_data_.size();
    return segment_data_[index];
  }

  point_type shift_;
  std::vector<point_type> point_data_;
  std::vector<segment_type> segment_data_;
  rect_type brect_;
  VB vb_;
  VD vd_;
  bool brect_initialized_;
  bool primary_edges_only_;
  bool internal_edges_only_;
};

class MainWindow : public QWidget {
  Q_OBJECT

 public:
  MainWindow() {
    glWidget_ = new GLWidget();
    file_dir_ = QDir(QDir::currentPath(), tr("*.txt"));
    file_name_ = tr("");

    QHBoxLayout* centralLayout = new QHBoxLayout;
    centralLayout->addWidget(glWidget_);
    centralLayout->addLayout(create_file_layout());
    setLayout(centralLayout);

    update_file_list();
    setWindowTitle(tr("Voronoi Visualizer"));
    layout()->setSizeConstraint(QLayout::SetFixedSize);
  }

 private slots:
  void primary_edges_only() {
    glWidget_->show_primary_edges_only();
  }

  void internal_edges_only() {
    glWidget_->show_internal_edges_only();
  }

  void browse() {
    QString new_path = QFileDialog::getExistingDirectory(
        0, tr("Choose Directory"), file_dir_.absolutePath());
    if (new_path.isEmpty()) {
      return;
    }
    file_dir_.setPath(new_path);
    update_file_list();
  }

  void build() {
    file_name_ = file_list_->currentItem()->text();
    QString file_path = file_dir_.filePath(file_name_);
    message_label_->setText("Building...");
    glWidget_->build(file_path);
    message_label_->setText("Double click the item to build voronoi diagram:");
    setWindowTitle(tr("Voronoi Visualizer - ") + file_path);
  }

  void print_scr() {
    if (!file_name_.isEmpty()) {
      QImage screenshot = glWidget_->grabFrameBuffer(true);
      QString output_file = file_dir_.absolutePath() + tr("/") +
          file_name_.left(file_name_.indexOf('.')) + tr(".png");
      screenshot.save(output_file, 0, -1);
    }
  }

 private:
  QGridLayout* create_file_layout() {
    QGridLayout* file_layout = new QGridLayout;

    message_label_ = new QLabel("Double click item to build voronoi diagram:");

    file_list_ = new QListWidget();
    file_list_->connect(file_list_,
                        SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                        this,
                        SLOT(build()));

    QCheckBox* primary_checkbox = new QCheckBox("Show primary edges only.");
    connect(primary_checkbox, SIGNAL(clicked()),
        this, SLOT(primary_edges_only()));

    QCheckBox* internal_checkbox = new QCheckBox("Show internal edges only.");
    connect(internal_checkbox, SIGNAL(clicked()),
        this, SLOT(internal_edges_only()));

    QPushButton* browse_button =
        new QPushButton(tr("Browse Input Directory"));
    connect(browse_button, SIGNAL(clicked()), this, SLOT(browse()));
    browse_button->setMinimumHeight(50);

    QPushButton* print_scr_button = new QPushButton(tr("Make Screenshot"));
    connect(print_scr_button, SIGNAL(clicked()), this, SLOT(print_scr()));
    print_scr_button->setMinimumHeight(50);

    file_layout->addWidget(message_label_, 0, 0);
    file_layout->addWidget(file_list_, 1, 0);
    file_layout->addWidget(primary_checkbox, 2, 0);
    file_layout->addWidget(internal_checkbox, 3, 0);
    file_layout->addWidget(browse_button, 4, 0);
    file_layout->addWidget(print_scr_button, 5, 0);

    return file_layout;
  }

  void update_file_list() {
    QFileInfoList list = file_dir_.entryInfoList();
    file_list_->clear();
    if (file_dir_.count() == 0) {
      return;
    }
    QFileInfoList::const_iterator it;
    for (it = list.begin(); it != list.end(); it++) {
      file_list_->addItem(it->fileName());
    }
    file_list_->setCurrentRow(0);
  }

  QDir file_dir_;
  QString file_name_;
  GLWidget* glWidget_;
  QListWidget* file_list_;
  QLabel* message_label_;
};

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.show();
  return app.exec();
}

#include "voronoi_visualizer.moc"
