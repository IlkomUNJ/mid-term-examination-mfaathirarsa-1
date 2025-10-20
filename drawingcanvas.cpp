#include <fstream>
#include <algorithm>
#include <vector>
#include "drawingcanvas.h"
#include <QColor>
#include <QPen>
#include <QDebug>

// Helper: merge overlapping rects (simple union-based merging)
static QVector<QRect> mergeOverlappingRects(const QVector<QRect>& rects) {
    if (rects.isEmpty()) return {};
    std::vector<QRect> work(rects.begin(), rects.end());
    std::sort(work.begin(), work.end(), [](const QRect &a, const QRect &b){
        if (a.x() != b.x()) return a.x() < b.x();
        if (a.y() != b.y()) return a.y() < b.y();
        if (a.width() != b.width()) return a.width() < b.width();
        return a.height() < b.height();
    });

    std::vector<QRect> merged;
    QRect cur = work[0];
    for (size_t i = 1; i < work.size(); ++i) {
        // if they intersect or are adjacent (expand by 1 to consider near-by hits)
        if (cur.intersects(work[i]) || cur.adjusted(-1,-1,1,1).intersects(work[i])) {
            cur = cur.united(work[i]);
        } else {
            merged.push_back(cur);
            cur = work[i];
        }
    }
    merged.push_back(cur);
    return QVector<QRect>(merged.begin(), merged.end());
}

DrawingCanvas::DrawingCanvas(QWidget *parent) : QWidget(parent) {
    // Set a minimum size for the canvas
    setMinimumSize(this->WINDOW_WIDTH, this->WINDOW_HEIGHT);
    // Set a solid background color
    setStyleSheet("background-color: white; border: 1px solid gray;");

    // --- Initialize an offscreen buffer for persistent drawing ---
    m_canvasBuffer = QPixmap(this->WINDOW_WIDTH, this->WINDOW_HEIGHT);
    m_canvasBuffer.fill(Qt::white); // start with a white background
}

void DrawingCanvas::clearPoints() {
    m_points.clear();
    m_detectedRects.clear();

    // --- Clear the canvas buffer too ---
    m_canvasBuffer.fill(Qt::white);

    update();
}

void DrawingCanvas::paintLines() {
    isPaintLinesClicked = true;

    // --- Ensure canvas buffer is valid and initialized in ARGB format ---
    if (m_canvasBuffer.isNull()) {
        m_canvasBuffer = QPixmap(this->WINDOW_WIDTH, this->WINDOW_HEIGHT);
        m_canvasBuffer.fill(Qt::white);
    }

    // Convert to ARGB32-backed QPixmap (avoid painting into an indexed format)
    QImage tmp = m_canvasBuffer.toImage().convertToFormat(QImage::Format_ARGB32);
    m_canvasBuffer = QPixmap::fromImage(tmp);
    // Clear before drawing lines (matching your previous behavior)
    m_canvasBuffer.fill(Qt::white);

    QPainter bufferPainter(&m_canvasBuffer);
    bufferPainter.setRenderHint(QPainter::Antialiasing, false); // disable smoothing for sharper lines

    QPen pen(Qt::red, 4);
    bufferPainter.setPen(pen);

    cout << "paint lines block is called" << endl;

    for (int i = 0; i + 1 < m_points.size(); i += 2)
        bufferPainter.drawLine(m_points[i], m_points[i + 1]);

    bufferPainter.end();
    update();
}


void DrawingCanvas::segmentDetection() {
    // Configurable thresholds (tweak if needed)
    const int redMin = 180;
    const int redGap = 50;      // red must exceed other channels by this
    const int looseThreshold = 7; // matches >= this count considered a match (original class code)

    QImage image = m_canvasBuffer.toImage().convertToFormat(QImage::Format_ARGB32);

    cout << "image width: " << image.width() << endl;
    cout << "image height: " << image.height() << endl;

    ofstream dumpFile("window_dump.txt");
    dumpFile << "=== Non-empty window dump for analysis ===\n";

    m_detectedRects.clear();
    int totalMatches = 0;

    // test multiple window sizes for analysis (dump non-empty for all sizes)
    vector<int> windowSizes = {3, 5, 7};

    for (int w : windowSizes) {
        int half = w / 2;
        dumpFile << "\n--- Testing window size: " << w << "x" << w << " ---\n";
        cout << "\n--- Testing window size: " << w << "x" << w << " ---\n";

        for (int i = half; i < image.width() - half; i++) {
            for (int j = half; j < image.height() - half; j++) {
                bool local_window[7][7] = {false}; // max size buffer
                bool nonEmpty = false;

                // build local window and detect red pixels
                for (int m = -half; m <= half; m++) {
                    for (int n = -half; n <= half; n++) {
                        QColor color(image.pixel(i + m, j + n));
                        bool isRed = (color.red() > redMin &&
                                      color.red() > color.green() + redGap &&
                                      color.red() > color.blue() + redGap);
                        local_window[m + half][n + half] = isRed;
                        if (isRed) nonEmpty = true;
                    }
                }

                // dump non-empty window for analysis
                if (nonEmpty) {
                    dumpFile << "Center (" << i << "," << j << ")\n";
                    for (int m = 0; m < w; ++m) {
                        for (int n = 0; n < w; ++n)
                            dumpFile << (local_window[m][n] ? "1 " : "0 ");
                        dumpFile << "\n";
                    }
                    dumpFile << "------\n";
                }

                // only pattern match on 3x3 for now (core algorithm)
                if (w == 3) {
                    bool vertical[3][3] = {
                        {0,1,0},
                        {0,1,0},
                        {0,1,0}
                    };
                    bool horizontal[3][3] = {
                        {0,0,0},
                        {1,1,1},
                        {0,0,0}
                    };
                    bool diagRight[3][3] = {
                        {1,0,0},
                        {0,1,0},
                        {0,0,1}
                    };
                    bool diagLeft[3][3] = {
                        {0,0,1},
                        {0,1,0},
                        {1,0,0}
                    };

                    vector<CustomMatrix> patterns = {
                        CustomMatrix(vertical),
                        CustomMatrix(horizontal),
                        CustomMatrix(diagRight),
                        CustomMatrix(diagLeft)
                    };

                    // Convert local_window[7][7] → 3x3 std::vector before passing
                    std::vector<std::vector<bool>> localVec(3, std::vector<bool>(3, false));
                    for (int a = 0; a < 3; ++a)
                        for (int b = 0; b < 3; ++b)
                            localVec[a][b] = local_window[a][b];

                    CustomMatrix current(localVec);

                    auto looseCompare = [&](const CustomMatrix& a, const CustomMatrix& b) {
                        int same = 0;
                        for (int ii = 0; ii < 3; ++ii)
                            for (int jj = 0; jj < 3; ++jj)
                                if (a.mat[ii][jj] == b.mat[ii][jj])
                                    same++;
                        return same >= looseThreshold;
                    };

                    for (const CustomMatrix& pattern : patterns) {
                        if (looseCompare(current, pattern)) {
                            m_detectedRects.append(QRect(i - 1, j - 1, 3, 3));
                            totalMatches++;
                            // log the matched window specifically for easier analysis
                            dumpFile << "MATCH (pattern) @ (" << i << "," << j << ")\n";
                            for (int a = 0; a < 3; ++a) {
                                for (int b = 0; b < 3; ++b)
                                    dumpFile << (localVec[a][b] ? "1 " : "0 ");
                                dumpFile << "\n";
                            }
                            dumpFile << "------\n";
                            break;
                        }
                    }
                }
            }
        }
    }

    dumpFile.close();
    cout << "Total detected segment windows (3x3 only): " << totalMatches << endl;

    // Merge overlapping detected rects to reduce duplicates
    if (!m_detectedRects.isEmpty()) {
        QVector<QRect> merged = mergeOverlappingRects(m_detectedRects);
        m_detectedRects = merged;
    }

    // Draw magenta rectangles (slightly enlarged) on m_canvasBuffer to visually mark candidates
    if (!m_detectedRects.isEmpty()) {
        QPainter p(&m_canvasBuffer);
        QPen pen(Qt::magenta, 1, Qt::DashLine);
        p.setPen(pen);
        for (const QRect& rect : std::as_const(m_detectedRects))
            p.drawRect(rect.adjusted(-1, -1, 1, 1)); // slightly bigger for visibility
        p.end();
    }

    update();
}


void DrawingCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // --- Draw the persistent canvas buffer first ---
    painter.drawPixmap(0, 0, m_canvasBuffer);

    // --- Draw stored points ---
    QPen pen(Qt::blue, 5);
    painter.setPen(pen);
    painter.setBrush(QBrush(Qt::blue));

    for (const QPoint& point : std::as_const(m_points))
        painter.drawEllipse(point, 3, 3);

    // --- Draw detected segment boxes ---
    if (!m_detectedRects.isEmpty()) {
        QPen detectPen(Qt::magenta, 1);
        painter.setPen(detectPen);
        for (const QRect& rect : std::as_const(m_detectedRects))
            painter.drawRect(rect);
    }
}

void DrawingCanvas::mousePressEvent(QMouseEvent *event) {
    m_points.append(event->pos());
    update();
}
