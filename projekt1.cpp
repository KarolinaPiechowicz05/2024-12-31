#include <iostream>
#include <cmath>

using namespace std;

bool czy_pierwsza(int n){
    for (int i = 2; i <= sqrt(n)+1; ++i){
        if (n % i == 0){
            return 0;
        }
    }
    return 1;
}
string nietak[] = {"Nie","Tak"};

int main() {
    int a, b;
    cout << "Podaj liczbe calkowita ";
    cin >> a >> b;
    int wyjscie;
    do {
        cout << endl;
        cout << endl;
        cout << "MENU" << endl;
        cout << "Podaj numer czynnosci, ktora chcesz wykonac" << endl;
        cout << "0. Wyjscie" << endl;
        cout << "2. Test pierwszosci" << endl;
        cin >> wyjscie;
        if (wyjscie == 2){
            cout << "Test pierwszosci dla liczby a=" << a << ": " << nietak[czy_pierwsza] << endl;
        }
    } while(wyjscie != 0);
    return 0;
}
