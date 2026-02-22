#pragma once

#include <QMainWindow>
#include <QPointer>
#include <memory>
#include <optional>

#include "TspInstance.h"
#include "Tour.h"

class TspWidget;
class QLabel;
class QPushButton;
class QSlider;
class QComboBox;
class QCheckBox;
class QThread;

class OptimizerWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void openTsp();
    void showProperties();
    void exportTour();

    void randomizeTour();
    void easyHeuristic();
    void thoroughHeuristic();

    void startOptimization();
    void stopOptimization();

    void viewOriginal();
    void viewBest();

    void onBestUpdated(const QVector<int>& bestOrder, double bestCost, double improvementPct);
    void onWorkerFinished();

    void onZoomChanged(int v);
    void onAngleChanged(int idx);
    void onShowLinesToggled(bool on);

private:
    void setLoadedState(bool loaded);
    void updateTitle();

    std::optional<TspInstance> m_instance;

    Tour m_original;
    Tour m_current;
    Tour m_best;

    double m_baseline = 0.0;

    // UI
    TspWidget* m_view = nullptr;

    QAction* m_actionOpen = nullptr;
    QAction* m_actionProps = nullptr;
    QAction* m_actionExport = nullptr;
    QAction* m_actionExit = nullptr;

    QAction* m_actionRandomize = nullptr;
    QAction* m_actionEasy = nullptr;
    QAction* m_actionThorough = nullptr;
    QAction* m_actionStart = nullptr;
    QAction* m_actionStop = nullptr;

    QAction* m_actionViewOriginal = nullptr;
    QAction* m_actionViewBest = nullptr;

    QLabel* m_improvementLabel = nullptr;
    QComboBox* m_methodCombo = nullptr;
    QPushButton* m_startStopButton = nullptr;
    QSlider* m_zoomSlider = nullptr;
    QComboBox* m_angleCombo = nullptr;
    QCheckBox* m_linesCheck = nullptr;

    // Worker thread
    QThread* m_thread = nullptr;
    OptimizerWorker* m_worker = nullptr;

    QString m_currentFile;
};
