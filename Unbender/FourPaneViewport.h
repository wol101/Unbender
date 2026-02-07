#ifndef FOURPANEVIEWPORT_H
#define FOURPANEVIEWPORT_H

#include <QWidget>
#include <QSplitter>
#include <array>

class FourPaneViewport : public QWidget
{
    Q_OBJECT

public:
    explicit FourPaneViewport(QWidget *pane0, QWidget *pane1, QWidget *pane2, QWidget *pane3, QWidget* parent = nullptr);
    ~FourPaneViewport() override;

    // Maximize a specific pane (0–3) or restore all
    void maximizePane(int index);
    void restoreLayout();
    bool isMaximized() const { return m_maximizedIndex >= 0; }
    void resetPaneSizes();

    QWidget* pane(int index) const { return m_panes[index]; }

    void setPanes(const std::array<QWidget *, 4> &newPanes);

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void buildLayout();
    void loadSettings();
    void saveSettings();
    void showContextMenu(QWidget* pane, const QPoint& globalPos);
    void syncVerticalSplitters(QSplitter* source, QSplitter* target);

    QSplitter* m_mainSplitter = nullptr;
    QSplitter* m_leftSplitter = nullptr;
    QSplitter* m_rightSplitter = nullptr;

    std::array<QWidget*,4> m_panes;

    int m_maximizedIndex = -1;   // -1 = normal mode
    bool m_syncing = false;
};

#endif // FOURPANEVIEWPORT_H
