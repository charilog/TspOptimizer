#include "MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QThread>

#include "TspWidget.h"
#include "optim/OptimizerWorker.h"
#include "optim/GeneticOptimizer.h"
#include "optim/SimAnnealOptimizer.h"
#include "optim/TwoOptOptimizer.h"
#include "optim/IlsOptimizer.h"

#include <fstream>

static QVector<int> toQVector(const std::vector<int>& v)
{
    QVector<int> out;
    out.reserve(static_cast<int>(v.size()));
    for (int x : v) out.push_back(x);
    return out;
}

static std::vector<int> identityOrder(int n)
{
    std::vector<int> ord(n);
    for (int i = 0; i < n; ++i) ord[i] = i;
    return ord;
}

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
{
    setWindowTitle(tr("TSP Route Optimizer"));

    // Menus
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    m_actionOpen   = fileMenu->addAction(tr("&Open TSP..."));
    m_actionExport = fileMenu->addAction(tr("&Export Tour..."));
    m_actionProps  = fileMenu->addAction(tr("&Properties..."));
    fileMenu->addSeparator();
    m_actionExit   = fileMenu->addAction(tr("E&xit"));

    auto* optMenu = menuBar()->addMenu(tr("&Optimize"));
    m_actionStart = optMenu->addAction(tr("&Run"));
    m_actionStop  = optMenu->addAction(tr("S&top"));
    optMenu->addSeparator();
    m_actionRandomize = optMenu->addAction(tr("&Random Tour"));
    m_actionEasy      = optMenu->addAction(tr("Insertion Heuristic (&Fast)"));
    m_actionThorough  = optMenu->addAction(tr("Farthest Insertion (&Thorough)"));

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    m_actionViewOriginal = viewMenu->addAction(tr("Show &Original Tour"));
    m_actionViewBest     = viewMenu->addAction(tr("Show &Best Tour"));

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    auto* aboutAct = helpMenu->addAction(tr("&About"));

    // Central view
    m_view = new TspWidget(this);
    setCentralWidget(m_view);

    // Status bar widgets (bottom controls)
    m_startStopButton = new QPushButton(tr("Stopped"), this);
    m_startStopButton->setEnabled(false);

    m_improvementLabel = new QLabel(tr("Improvement: 0%"), this);
    m_improvementLabel->setEnabled(false);

    m_methodCombo = new QComboBox(this);
    m_methodCombo->addItem(QStringLiteral("Genetic Algorithm (GA)"));
    m_methodCombo->addItem(QStringLiteral("Simulated Annealing (SA)"));
    m_methodCombo->addItem(QStringLiteral("2-opt Local Search"));
    m_methodCombo->addItem(QStringLiteral("Iterated Local Search (ILS)"));
    m_methodCombo->setEnabled(false);

    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setRange(1, 50);
    m_zoomSlider->setValue(10);
    m_zoomSlider->setEnabled(false);

    m_angleCombo = new QComboBox(this);
    m_angleCombo->addItems({QStringLiteral("0"), QStringLiteral("90"), QStringLiteral("180"), QStringLiteral("270")});
    m_angleCombo->setEnabled(false);

    m_linesCheck = new QCheckBox(tr("Edges"), this);
    m_linesCheck->setChecked(false);
    m_linesCheck->setEnabled(false);

    statusBar()->addWidget(m_startStopButton);
    statusBar()->addWidget(m_improvementLabel);
    statusBar()->addWidget(new QLabel(tr("Method:"), this));
    statusBar()->addWidget(m_methodCombo);
    statusBar()->addWidget(new QLabel(QStringLiteral("Zoom:"), this));
    statusBar()->addWidget(m_zoomSlider);
    statusBar()->addWidget(new QLabel(tr("Angle:"), this));
    statusBar()->addWidget(m_angleCombo);
    statusBar()->addWidget(m_linesCheck);

    // Connections
    connect(m_actionOpen,   &QAction::triggered, this, &MainWindow::openTsp);
    connect(m_actionProps,  &QAction::triggered, this, &MainWindow::showProperties);
    connect(m_actionExport, &QAction::triggered, this, &MainWindow::exportTour);
    connect(m_actionExit,   &QAction::triggered, this, &QWidget::close);

    connect(m_actionRandomize, &QAction::triggered, this, &MainWindow::randomizeTour);
    connect(m_actionEasy,      &QAction::triggered, this, &MainWindow::easyHeuristic);
    connect(m_actionThorough,  &QAction::triggered, this, &MainWindow::thoroughHeuristic);
    connect(m_actionStart,     &QAction::triggered, this, &MainWindow::startOptimization);
    connect(m_actionStop,      &QAction::triggered, this, &MainWindow::stopOptimization);

    connect(m_actionViewOriginal, &QAction::triggered, this, &MainWindow::viewOriginal);
    connect(m_actionViewBest,     &QAction::triggered, this, &MainWindow::viewBest);

    connect(aboutAct, &QAction::triggered, this, [this](){
        QMessageBox::information(this,
                                 tr("About"),
                                 tr("TSP Route Optimizer\n\nTravelling Salesman Problem\n\n(C++/Qt6 rewrite)"));
    });

    connect(m_startStopButton, &QPushButton::clicked, this, [this](){
        if (m_thread) stopOptimization();
        else startOptimization();
    });

    connect(m_zoomSlider, &QSlider::valueChanged, this, &MainWindow::onZoomChanged);
    connect(m_angleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onAngleChanged);
    connect(m_linesCheck, &QCheckBox::toggled, this, &MainWindow::onShowLinesToggled);

    setLoadedState(false);
}

MainWindow::~MainWindow()
{
    stopOptimization();
}

void MainWindow::setLoadedState(bool loaded)
{
    m_actionProps->setEnabled(loaded);
    m_actionExport->setEnabled(loaded);

    m_actionRandomize->setEnabled(loaded);
    m_actionEasy->setEnabled(loaded);
    m_actionThorough->setEnabled(loaded);

    m_actionStart->setEnabled(loaded);
    m_actionStop->setEnabled(loaded);

    m_actionViewOriginal->setEnabled(loaded);
    m_actionViewBest->setEnabled(loaded);

    m_startStopButton->setEnabled(loaded);
    m_improvementLabel->setEnabled(loaded);
    m_methodCombo->setEnabled(loaded);
    m_zoomSlider->setEnabled(loaded);
    m_angleCombo->setEnabled(loaded);
    m_linesCheck->setEnabled(loaded);

    if (!loaded)
    {
        m_startStopButton->setText(tr("Stopped"));
        m_improvementLabel->setText(tr("Improvement: 0%"));
    }
}

void MainWindow::updateTitle()
{
    const QString base = tr("TSP Route Optimizer");
    if (m_currentFile.isEmpty())
        setWindowTitle(base);
    else
        setWindowTitle(base + QStringLiteral(" - ") + m_currentFile);
}

void MainWindow::openTsp()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Open"),
                                                      QString(),
                                                      QStringLiteral("TSP Files (*.tsp)"));
    if (path.isEmpty())
        return;

    stopOptimization();

    try
    {
        m_instance = TspInstance::loadFromTspFile(path.toStdString());

        m_original = Tour(&(*m_instance));
        m_current  = m_original;
        m_best     = m_original;

        m_baseline = m_original.cost();

        m_currentFile = QFileInfo(path).fileName();
        updateTitle();

        m_view->setInstance(&(*m_instance));
        m_view->setBorderScale(static_cast<double>(m_zoomSlider->value()) / 10.0);
        m_view->setRotationDeg(m_angleCombo->currentText().toInt());
        m_view->setShowLines(m_linesCheck->isChecked());
        m_view->setTour(toQVector(m_current.order()));
        m_view->clearLastTour();

        setLoadedState(true);
    }
    catch (const std::exception& ex)
    {
        QMessageBox::critical(this, tr("Error"), QString::fromUtf8(ex.what()));
        setLoadedState(false);
    }
}

void MainWindow::showProperties()
{
    if (!m_instance)
    {
        QMessageBox::critical(this, tr("Error"), tr("Please open a TSP file first."));
        return;
    }

    const int n = m_instance->size();
    const int km = static_cast<int>(m_original.cost() / 10000.0);
    QMessageBox::information(this,
                             m_currentFile,
                             tr("%1 cities\n\nTour length: %2 km").arg(n).arg(km));
}

void MainWindow::exportTour()
{
    if (!m_instance)
        return;

    // Ensure stop (so m_best is stable)
    stopOptimization();

    const QString path = QFileDialog::getSaveFileName(this,
                                                      tr("Export"),
                                                      QString(),
                                                      QStringLiteral("TOUR Files (*.tour)"));
    if (path.isEmpty())
        return;

    QString outPath = path;
    if (!outPath.endsWith(".tour", Qt::CaseInsensitive))
        outPath += ".tour";

    std::ofstream out(outPath.toStdString());
    if (!out.is_open())
    {
        QMessageBox::critical(this, tr("Error"), tr("Could not write the output file."));
        return;
    }

    const auto& pts = m_instance->points();
    const auto& ord = m_best.order();

    for (int i = 0; i < static_cast<int>(ord.size()); ++i)
    {
        const auto& p = pts[ord[i]];
        out << (i + 1) << " " << (static_cast<double>(p.x) / 10000.0) << " " << (static_cast<double>(p.y) / 10000.0) << "\n";
    }
    out.flush();
}

void MainWindow::randomizeTour()
{
    if (!m_instance) return;
    stopOptimization();

    std::mt19937 rng(std::random_device{}());
    m_current.randomize(10000, rng);

    if (m_current.cost() < m_best.cost())
        m_best = m_current;

    m_view->setTour(toQVector(m_current.order()));
}

void MainWindow::easyHeuristic()
{
    if (!m_instance) return;
    stopOptimization();

    m_current.easyHeuristic();
    if (m_current.cost() < m_best.cost())
        m_best = m_current;

    m_view->setTour(toQVector(m_current.order()));
}

void MainWindow::thoroughHeuristic()
{
    if (!m_instance) return;
    stopOptimization();

    m_current.thoroughHeuristic();
    if (m_current.cost() < m_best.cost())
        m_best = m_current;

    m_view->setTour(toQVector(m_current.order()));
}

void MainWindow::startOptimization()
{
    if (!m_instance || m_thread)
        return;

    // baseline always refers to the original tour (same as Java)
    m_baseline = m_original.cost();

    std::unique_ptr<IOptimizer> optimizer;
    const int method = m_methodCombo->currentIndex();

    switch (method)
    {
        case 0: optimizer = std::make_unique<GeneticOptimizer>(m_current, 30, 2); break;
        case 1: optimizer = std::make_unique<SimAnnealOptimizer>(m_current); break;
        case 2: optimizer = std::make_unique<TwoOptOptimizer>(m_current); break;
        case 3: optimizer = std::make_unique<IlsOptimizer>(m_current); break;
        default: optimizer = std::make_unique<SimAnnealOptimizer>(m_current); break;
    }

    m_thread = new QThread(this);
    m_worker = new OptimizerWorker(std::move(optimizer));

    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &OptimizerWorker::run);
    connect(m_worker, &OptimizerWorker::bestUpdated, this, &MainWindow::onBestUpdated, Qt::QueuedConnection);
    connect(m_worker, &OptimizerWorker::finished, this, &MainWindow::onWorkerFinished, Qt::QueuedConnection);
    connect(m_worker, &OptimizerWorker::finished, m_thread, &QThread::quit);

    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    m_startStopButton->setText(tr("Running"));
    m_thread->start();
}

void MainWindow::stopOptimization()
{
    if (!m_thread)
        return;

    if (m_worker)
        m_worker->stop();

    // thread will quit when worker emits finished()
    // but we also force a polite interruption in case it is in a long iterate
    m_thread->requestInterruption();
    m_thread->quit();
    m_thread->wait();

    m_thread = nullptr;
    m_worker = nullptr;

    m_startStopButton->setText(tr("Stopped"));

    // make current equal to best at stop (like Java behavior)
    if (m_instance)
    {
        m_current = m_best;
        m_view->setTour(toQVector(m_current.order()));
    }
}

void MainWindow::viewOriginal()
{
    if (!m_instance) return;
    stopOptimization();
    m_current = m_original;
    m_view->clearLastTour();
    m_view->setTour(toQVector(m_current.order()));
}

void MainWindow::viewBest()
{
    if (!m_instance) return;
    stopOptimization();
    m_current = m_best;
    m_view->clearLastTour();
    m_view->setTour(toQVector(m_current.order()));
}

void MainWindow::onBestUpdated(const QVector<int>& bestOrder, double bestCost, double improvementPct)
{
    if (!m_instance) return;

    // update best tour
std::vector<int> ord;
ord.reserve(bestOrder.size());
for (int v : bestOrder) ord.push_back(v);
m_best = Tour(&(*m_instance), std::move(ord));
    // keep cost if we wanted; Tour evaluates already.
    (void)bestCost;

    // show the best in the view (blue), leaving last (red)
    m_view->setTour(bestOrder);

    m_improvementLabel->setText(tr("Improvement: %1%").arg(improvementPct, 0, 'f', 3));
}

void MainWindow::onWorkerFinished()
{
    // worker finished, thread is already quitting
}

void MainWindow::onZoomChanged(int v)
{
    m_view->setBorderScale(static_cast<double>(v) / 10.0);
}

void MainWindow::onAngleChanged(int)
{
    m_view->setRotationDeg(m_angleCombo->currentText().toInt());
}

void MainWindow::onShowLinesToggled(bool on)
{
    m_view->setShowLines(on);
}
