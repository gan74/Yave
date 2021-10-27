/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/


#include "EventHandler.h"

namespace yave {

const char* key_name(Key key) {
#define CASE_KEY(key) case Key::key: return #key;
    switch(key) {
        CASE_KEY(Unknown)
        CASE_KEY(Tab)
        CASE_KEY(Clear)
        CASE_KEY(Backspace)
        CASE_KEY(Enter)
        CASE_KEY(Escape)
        CASE_KEY(PageUp)
        CASE_KEY(PageDown)
        CASE_KEY(End)
        CASE_KEY(Home)
        CASE_KEY(Left)
        CASE_KEY(Up)
        CASE_KEY(Right)
        CASE_KEY(Down)
        CASE_KEY(Insert)
        CASE_KEY(Delete)
        CASE_KEY(Space)
        CASE_KEY(A)
        CASE_KEY(B)
        CASE_KEY(C)
        CASE_KEY(D)
        CASE_KEY(E)
        CASE_KEY(F)
        CASE_KEY(G)
        CASE_KEY(H)
        CASE_KEY(I)
        CASE_KEY(J)
        CASE_KEY(K)
        CASE_KEY(L)
        CASE_KEY(M)
        CASE_KEY(N)
        CASE_KEY(O)
        CASE_KEY(P)
        CASE_KEY(Q)
        CASE_KEY(R)
        CASE_KEY(S)
        CASE_KEY(T)
        CASE_KEY(U)
        CASE_KEY(V)
        CASE_KEY(W)
        CASE_KEY(X)
        CASE_KEY(Y)
        CASE_KEY(Z)
        CASE_KEY(F1)
        CASE_KEY(F2)
        CASE_KEY(F3)
        CASE_KEY(F4)
        CASE_KEY(F5)
        CASE_KEY(F6)
        CASE_KEY(F7)
        CASE_KEY(F8)
        CASE_KEY(F9)
        CASE_KEY(F10_Reserved)
        CASE_KEY(F11)
        CASE_KEY(F12)
        CASE_KEY(Alt)
        CASE_KEY(Ctrl)

        default:
        break;
    }
#undef CASE_KEY
    return "";
}

}


