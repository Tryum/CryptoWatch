#pragma once

#include <QObject>
#include <QVariantMap>

class CoinbasePrivate;

class Coinbase : public QObject
{
    Q_OBJECT
public:
    Coinbase(QString refreshToken = "", QObject* parent = nullptr);
    ~Coinbase();

public slots:
    void grant() const;
    void getCurrencies() const ;
    void getAccounts() const;

    QString refreshToken() const;

signals:
    void onAccessGranted();
    void onAccounts(QVariantMap accounts);
    void onCurrencies(QVariantMap currencies);

private:
    CoinbasePrivate* coinbasePrivate;
};
