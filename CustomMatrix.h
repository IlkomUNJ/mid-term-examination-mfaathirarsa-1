#ifndef CUSTOMMATRIX_H
#define CUSTOMMATRIX_H

#include <vector>
using namespace std;

class CustomMatrix {
public:
    vector<vector<bool>> mat;
    int size;

    CustomMatrix() : size(3) {
        mat = vector<vector<bool>>(3, vector<bool>(3, false));
    }

    CustomMatrix(bool m[3][3]) {
        size = 3;
        mat = vector<vector<bool>>(3, vector<bool>(3, false));
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                mat[i][j] = m[i][j];
    }

    CustomMatrix(const vector<vector<bool>>& m) {
        size = static_cast<int>(m.size());
        mat = m;
    }

    void fillMatrix(const vector<vector<bool>>& m) {
        size = static_cast<int>(m.size());
        mat = m;
    }
};

#endif // CUSTOMMATRIX_H
