#define WIN32_LEAN_AND_MEAN
#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStackedLayout>
#include <QString>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <Windows.h>

using namespace Qt::Literals;

namespace {

QWindow *settingsWindow = nullptr;

class MyTabButton : public QWidget {
public:
  MyTabButton(QStackedLayout *layout, QWidget *page, int myIndex)
      : layout_(layout), page_(page), myIndex_(myIndex) {
    QObject::connect(
        layout, &QStackedLayout::currentChanged, this, [this](int next) {
          if (next == this->myIndex_) {
            for (auto *obj : this->parentWidget()->children()) {
              auto *wid = dynamic_cast<QWidget *>(obj);
              if (!wid || wid == this) {
                continue;
              }
              wid->setStyleSheet(u"color: #fff"_s);
            }
            this->setStyleSheet(u"background: #222; color: #4FC3F7;"_s);
          } else {
            this->setStyleSheet(u"color: #fff"_s);
          }
        });
  }

  void paintEvent(QPaintEvent * /*event*/) override {
    QPainter painter(this);

    QStyleOption opt;
    opt.initFrom(this);

    this->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int pad = (3 * ((this->height() - 20) / 2)) + 20;
    this->style()->drawItemText(
        &painter, QRect(pad, 0, width() - pad, height()),
        Qt::AlignLeft | Qt::AlignVCenter, this->palette(), false, "My Tab");
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() != Qt::LeftButton) {
      return;
    }

    this->setFocus();
    this->layout_->setCurrentWidget(this->page_);
  }

private:
  QStackedLayout *layout_;
  QWidget *page_;
  int myIndex_ = -1;
};

class MyPage : public QWidget {
public:
  MyPage() { (new QVBoxLayout(this))->addWidget(new QLabel("Hi there!")); }
};

void breakHere() { _CrtDbgBreak(); }

#define bail(v)                                                                \
  if (!(v)) {                                                                  \
    breakHere();                                                               \
    return;                                                                    \
  }

QLayout *lcLayout(QLayoutItem *in) {
  if (!in) {
    return nullptr;
  }
  auto *wid = in->widget();
  if (!wid) {
    return nullptr;
  }
  return wid->layout();
}

void initWindow() {
  auto *win = qApp->activeWindow();
  auto *outerBox = win->layout();
  bail(outerBox);

  auto *centerBox = lcLayout(outerBox->itemAt(1));
  bail(centerBox);
  bail(centerBox->count() >= 2);

  auto *tabContainer =
      dynamic_cast<QVBoxLayout *>(lcLayout(centerBox->itemAt(0)));
  auto *pageStack =
      dynamic_cast<QStackedLayout *>(lcLayout(centerBox->itemAt(1)));
  bail(tabContainer && pageStack);

  auto *page = new MyPage;
  auto *btn = new MyTabButton(pageStack, page, pageStack->addWidget(page));
  btn->setFixedHeight(30);
  tabContainer->addWidget(btn, 0, Qt::AlignTop);
}

} // namespace

auto WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD fdwReason,
                    LPVOID /*lpvReserved*/) -> BOOL {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    QObject::connect(qApp, &QGuiApplication::focusWindowChanged,
                     [](QWindow *window) {
                       if (settingsWindow || !window) {
                         return;
                       }
                       if (window->title() == u"Chatterino Settings"_s) {
                         settingsWindow = window;
                         initWindow();
                       }
                     });
  }

  return TRUE;
}
