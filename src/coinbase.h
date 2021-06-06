#pragma once

#include <QObject>
#include <QVariantMap>

class CoinbasePrivate;

class Coinbase : public QObject
{
    Q_OBJECT
public:
    Coinbase(QObject* parent = nullptr);
    ~Coinbase();

public slots:
    void grant() const;
    void getCurrencies() const ;
    void getAccounts() const;

signals:
    void onAccessGranted();
    void onAccounts(QVariantMap accounts);
    void onCurrencies(QVariantMap currencies);

private:
    CoinbasePrivate* coinbasePrivate;
};
