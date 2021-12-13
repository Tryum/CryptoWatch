#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class BinancePrivate;

class Binance : public QObject {
          Q_OBJECT
public:
    Binance(QString key, QString secret, QObject* parent=nullptr);
    ~Binance();

public slots:
    void getCurrencies() const ;
    void getAccounts() const;

signals:
    void onAccounts(QVariantMap accounts);
    void onCurrencies(QVariantMap currencies);

private:
    BinancePrivate* binance;
};
