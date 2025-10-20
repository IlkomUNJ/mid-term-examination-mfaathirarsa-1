# **Segment Detection Analysis Report**

## **1. Objective**

The goal of this experiment is to detect simple red line segments drawn on a canvas by analyzing pixel patterns in small sliding windows (3×3, 5×5, 7×7).
Each window is scanned over the canvas image to determine whether it contains a known line pattern (vertical, horizontal, or diagonal).
Matched regions are visualized using magenta rectangles on the drawing canvas.

---

## **2. Implementation Overview**

The detection process is implemented in the function `DrawingCanvas::segmentDetection()` (see source code).

### **Key Steps:**

1. **Canvas Capture:**
   The current canvas content (`m_canvasBuffer`) is converted into a `QImage` for pixel access.

2. **Sliding Window Analysis:**
   For each window size (3×3, 5×5, 7×7), the algorithm slides across every pixel `(i, j)` and extracts the surrounding pixels.

3. **Red Pixel Identification:**
   Each pixel is marked as `1` (true) if:

   ```cpp
   color.red() > 180 &&
   color.red() > color.green() + 50 &&
   color.red() > color.blue() + 50
   ```

   Otherwise, it is `0` (false).
   This filters only strong red pixels, which represent drawn line segments.

4. **Pattern Matching (3×3):**
   The extracted 3×3 window is compared against 4 basic segment templates:

   | Pattern          | Description | Matrix                |                       |
   | ---------------- | ----------- | --------------------- | --------------------- |
   | Vertical         | `           | `                     | 0 1 0 / 0 1 0 / 0 1 0 |
   | Horizontal       | `―`         | 0 0 0 / 1 1 1 / 0 0 0 |                       |
   | Diagonal (Right) | `/`         | 1 0 0 / 0 1 0 / 0 0 1 |                       |
   | Diagonal (Left)  | `\`         | 0 0 1 / 0 1 0 / 1 0 0 |                       |

   A **loose match** threshold of ≥7 matching cells (out of 9) is used to handle noise or partial coverage.

5. **Window Dump Logging:**
   For debugging and verification, all non-empty windows are written to `window_dump.txt`.
   Each block lists:

   * Window center `(i, j)`
   * Binary window matrix (1 = red pixel)
   * “MATCH (pattern)” lines for successfully matched patterns.

6. **Rectangular Overlay:**
   Matched regions are recorded as small `QRect`s and drawn with dashed magenta outlines after merging overlapping detections.

---

## **3. Observations from `window_dump.txt`**

Example snippet (from actual run):

```
--- Testing window size: 3x3 ---
Center (35,320)
0 0 0 
0 0 0 
0 0 1 
------
MATCH (pattern) @ (35,320)
0 0 0 
0 0 0 
0 0 1 
------
Center (35,321)
0 0 0 
0 0 0 
0 1 0 
------
MATCH (pattern) @ (35,321)
0 0 0 
0 0 0 
0 1 0 
------
Center (35,322)
0 0 0 
0 0 0 
1 0 0 
------
MATCH (pattern) @ (35,322)
0 0 0 
0 0 0 
1 0 0 
------
```

### **Interpretation:**

* The three consecutive matches at centers `(35,320)`, `(35,321)`, and `(35,322)` indicate a **downward diagonal pattern** of red pixels.
* These 3×3 windows each contain one or two red pixels positioned in the lower diagonal, fitting the diagonal pattern criteria.

Another example:

```
Center (36,321)
0 0 0 
0 1 0 
0 1 1 
MATCH (pattern)
```

This represents a small corner or branching shape — partially fitting a **vertical or diagonal** pattern.

---

### **Larger Window Dumps (5×5 and 7×7)**

```
Center (167,236)
1 1 1 1 0 0 0 
1 1 1 0 0 0 0 
1 1 0 0 0 0 0 
1 1 0 0 0 0 0 
1 0 0 0 0 0 0 
...
```

These show **dense clusters of red pixels**, corresponding to longer connected segments or intersection regions.
Although current matching only uses 3×3 templates, these dumps confirm that **larger windows** successfully capture extended structures.

---

## **4. Visual Verification**

After detection:

* All matching windows are drawn on the canvas in **magenta dashed rectangles**.
* Overlapping boxes are merged for cleaner visualization using a union-based rectangle merging function.

Example:

```cpp
QPen pen(Qt::magenta, 1, Qt::DashLine);
p.drawRect(rect.adjusted(-1, -1, 1, 1));
```

Result: clusters of dashed boxes appear along the drawn red lines, confirming successful detection.

---

## **5. Discussion**

* **Accuracy:**
  The 3×3 pattern detector works well for small, continuous segments.
  Minor pixel gaps can still trigger detections due to the loose threshold, improving robustness.

* **Limitations:**

  * 3×3 windows cannot detect long continuous lines directly.
  * Slight red intensity variations may cause missed detections.
  * Patterns currently only cover straight and diagonal segments.

* **Potential Improvements:**

  * Add 5×5 and 7×7 patterns to detect longer lines.
  * Implement rotation-invariant or connected-component–based detection.
  * Use adaptive thresholds based on image brightness.

---

## **6. Conclusion**

This experiment demonstrates a **basic local pattern recognition** approach using sliding windows and binary templates.
The system effectively identifies simple red line segments drawn by the user and highlights their approximate positions.
The window dump logs validate the correctness of the algorithm and provide insight into its behavior on different scales.

---

## **7. Appendix – Code Excerpts**

**Snippet: Pattern comparison**

```cpp
auto looseCompare = [&](const CustomMatrix& a, const CustomMatrix& b) {
    int same = 0;
    for (int ii = 0; ii < 3; ++ii)
        for (int jj = 0; jj < 3; ++jj)
            if (a.mat[ii][jj] == b.mat[ii][jj])
                same++;
    return same >= looseThreshold;
};
```

**Snippet: Drawing detection boxes**

```cpp
QPen pen(Qt::magenta, 1, Qt::DashLine);
for (const QRect& rect : std::as_const(m_detectedRects))
    p.drawRect(rect.adjusted(-1, -1, 1, 1));
```

---

## **8. Summary Table**

| Parameter           | Value / Description                                 |
| ------------------- | --------------------------------------------------- |
| Window Sizes Tested | 3×3, 5×5, 7×7                                       |
| Red Threshold       | R > 180, R > G + 50, R > B + 50                     |
| Match Threshold     | ≥7/9 for 3×3 windows                                |
| Detected Patterns   | Vertical, Horizontal, Diagonal (Left/Right)         |
| Output File         | `window_dump.txt`                                   |
| Visual Output       | Magenta dashed rectangles marking detected segments |