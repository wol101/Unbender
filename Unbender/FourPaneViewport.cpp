#include "FourPaneViewport.h"

#include <QVBoxLayout>
#include <QSettings>
#include <QEvent>
#include <QMenu>
#include <QContextMenuEvent>

static const char* SETTINGS_GROUP = "FourPaneViewport";

FourPaneViewport::FourPaneViewport(QWidget *pane0, QWidget *pane1, QWidget *pane2, QWidget *pane3, QWidget* parent)
    : QWidget(parent)
{
    m_panes[0] = pane0;
    m_panes[1] = pane1;
    m_panes[2] = pane2;
    m_panes[3] = pane3;
    buildLayout();
    resetPaneSizes();
}

FourPaneViewport::~FourPaneViewport()
{
    saveSettings();
}

void FourPaneViewport::buildLayout()
{
    // this enables the context menu support
    for (int i = 0; i < 4; ++i)
        m_panes[i]->installEventFilter(this);

    // Left vertical splitter
    m_leftSplitter = new QSplitter(Qt::Vertical, this);
    m_leftSplitter->addWidget(m_panes[0]); // TL
    m_leftSplitter->addWidget(m_panes[2]); // BL

    // Right vertical splitter
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_rightSplitter->addWidget(m_panes[1]); // TR
    m_rightSplitter->addWidget(m_panes[3]); // BR

    // Main horizontal splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);

    connect(m_leftSplitter, &QSplitter::splitterMoved, this, [this](int, int){ syncVerticalSplitters(m_leftSplitter, m_rightSplitter); });
    connect(m_rightSplitter, &QSplitter::splitterMoved, this, [this](int, int){ syncVerticalSplitters(m_rightSplitter, m_leftSplitter); });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_mainSplitter);
}

void FourPaneViewport::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    loadSettings();
}

void FourPaneViewport::loadSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    s.beginGroup(SETTINGS_GROUP);

    if (s.contains("main"))
        m_mainSplitter->restoreState(s.value("main").toByteArray());
    if (s.contains("left"))
        m_leftSplitter->restoreState(s.value("left").toByteArray());
    if (s.contains("right"))
        m_rightSplitter->restoreState(s.value("right").toByteArray());

    s.endGroup();
}

void FourPaneViewport::saveSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "Unbender");
    s.beginGroup(SETTINGS_GROUP);

    s.setValue("main",  m_mainSplitter->saveState());
    s.setValue("left",  m_leftSplitter->saveState());
    s.setValue("right", m_rightSplitter->saveState());

    s.endGroup();
}

void FourPaneViewport::maximizePane(int index)
{
    if (index < 0 || index > 3)
        return;

    if (m_maximizedIndex == index)
        return;

    m_maximizedIndex = index;

    saveSettings(); // make sure the saved states are up to date

    // Hide all other panes
    for (int i = 0; i < 4; ++i)
        m_panes[i]->setVisible(i == index);

    // Expand the splitter chain so the visible pane fills everything
    m_mainSplitter->setSizes({1,1});
    m_leftSplitter->setSizes({1,1});
    m_rightSplitter->setSizes({1,1});
}

void FourPaneViewport::restoreLayout()
{
    if (m_maximizedIndex < 0)
        return;

    m_maximizedIndex = -1;

    // Show all panes again
    for (auto* p : m_panes)
        p->setVisible(true);

    loadSettings(); // restore original splitter sizes
}

bool FourPaneViewport::eventFilter(QObject* obj, QEvent* event)
{
    // Right-click context menu
    if (event->type() == QEvent::ContextMenu) {
        auto* ce = static_cast<QContextMenuEvent*>(event);

        for (int i = 0; i < 4; ++i) {
            if (obj == m_panes[i]) {
                showContextMenu(m_panes[i], ce->globalPos());
                return true;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void FourPaneViewport::showContextMenu(QWidget* pane, const QPoint& globalPos)
{
    QMenu menu;

    int index = -1;
    for (int i = 0; i < 4; ++i)
        if (pane == m_panes[i])
            index = i;

    if (index < 0)
        return;

    if (!isMaximized()) {
        QAction* maxAct = menu.addAction("Maximize Pane");
        connect(maxAct, &QAction::triggered, this, [this, index]() {
            maximizePane(index);
        });
    } else {
        QAction* restoreAct = menu.addAction("Restore Layout");
        connect(restoreAct, &QAction::triggered, this, [this]() {
            restoreLayout();
        });
    }

    menu.exec(globalPos);
}

void FourPaneViewport::syncVerticalSplitters(QSplitter* source, QSplitter* target)
{
    if (m_syncing)
        return;

    m_syncing = true;
    target->setSizes(source->sizes());
    m_syncing = false;
}

void FourPaneViewport::resetPaneSizes()
{
    // Reset horizontal splitter (left vs right)
    m_mainSplitter->setSizes({1, 1});

    // Reset vertical splitters (top vs bottom)
    m_leftSplitter->setSizes({1, 1});
    m_rightSplitter->setSizes({1, 1});

    // If you sync vertical splitters, ensure they stay in sync
    if (!m_syncing) {
        m_syncing = true;
        m_rightSplitter->setSizes(m_leftSplitter->sizes());
        m_syncing = false;
    }
}

