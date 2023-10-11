#include <Arduino.h>

#pragma once

#ifndef Payment_h
#define Payment_h

struct Withdrawal {
    String tag;
    String callback;
    String k1;
    int minWithdrawable;
    int maxWithdrawable;
    String defaultDescription;
};

struct Invoice {
    String paymentHash;
    String paymentRequest;
    String checkingId;
    String lnurlResponse;
};

class Payment {
    public:

        Payment();
        void configure(const char*  amount, const char*  lnbitsServer, const char*  invoiceKey);
        int getVendingPrice();
        bool payWithLnUrlWithdrawl(String url);

    private:

        String _amount;
        String _lnbitsServer;
        String _invoiceKey;

        String _dataId;
        String _description;
        String _payReq;

        bool _down;
        bool _paid;

        bool withdraw(String callback, String k1, String pr);
        Withdrawal getWithdrawal(String uri);
        bool isAmountInWithdrawableBounds(int amount, int minWithdrawable, int maxWithdrawable);
        Invoice getInvoice(String description);
        bool checkInvoice(String invoiceId);
        String decode(String lnUrl);
        String getUrl(String string);
};

#endif
