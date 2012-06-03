#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <mysql/mysql.h>
#include "iniparser.h"
#include "log4c.h"

#define NVPAPI_HOST "https://api-3t.paypal.com/nvp"
#define NVPVERSION "51.0"
#define NVPREQTS "METHOD=%s&VERSION=%s&PWD=%s&USER=%s&SIGNATURE=%s&STARTDATE=%s&ENDDATE=%s"
#define NVPREQGTD "METHOD=%s&VERSION=%s&PWD=%s&USER=%s&SIGNATURE=%s&TRANSACTIONID=%s"
#define INSERT_TRA "insert into transaction (time_stamp,time_zone,type,email,name,transaction_id,status,amt,currency_code,fee_amt,net_amt) values (?,?,?,?,?,?,?,?,?,?,?)"
#define INSERT_TRA_DETAIL "INSERT INTO `transaction_detail` (`receiver_business` ,`receiver_email` ,`receiver_id` ,`email` ,`payer_id` ,`payer_status` ,`country_code` , \
`ship_to_name` ,`ship_street` ,`ship_to_city` ,`ship_to_state` ,`ship_to_country_code` ,`ship_to_country_name` ,`ship_to_zip` ,`address_owner` ,`address_status` ,`sales_tax` , \
`buyer_id` ,`closing_date` ,`time_stamp` ,`correlation_id` ,`ack` ,`version` ,`build` ,`first_name` ,`last_name` ,`transaction_id` ,`transaction_type` ,`payment_type` ,`order_time` , \
`amt` ,`fee_amt` ,`tax_amt` ,`currency_code` ,`payment_status` ,`pending_reason` ,`reason_code` ,`shipping_method`) VALUES (\
?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
#define TRA_SIZE 1111
#define TRA_PAGE_SIZE 100
#define TRA_LEN 11
#define TRA_DETAIL_SIZE 100

struct paypal_info {
	char *user_name;
	char *password;
	char *signature;
};

struct paypal_info_array {
	struct paypal_info **array;
	int length;
};

struct string {
	char *ptr;
	size_t len;
};

struct two_array {
	char **array;
	int length;
};

struct transaction_array {
	struct transaction **tra;
	int length;
};

struct fields {
	char *name;
	char *value;
	struct fields *next;
};

struct transaction {
	char *time_stamp;
	char *time_zone;
	char *type;
	char *email;
	char *name;
	char *transaction_id;
	char *status;
	char *amt;
	char *currency_code;
	char *fee_amt;
	char *net_amt;
};

struct item {
	char *name;
	char *number;
	int qty;
	char *currency_code;
	double amt;
	struct item *next;
};

struct transaction_detail {
	char *receiver_business;
	char *receiver_email;
	char *receiver_id;
	char *email;
	char *payer_id;
	char *payer_status;
	char *country_code;
	char *ship_to_name;
	char *ship_street;
	char *ship_to_city;
	char *ship_to_state;
	char *ship_to_country_code;
	char *ship_to_country_name;
	char *ship_to_zip;
	char *address_owner;
	char *address_status;
	char *sales_tax;
	char *buyer_id;
	char *closing_date;
	char *time_stamp;
	char *correlation_id;
	char *ack;
	char *version;
	char *build;
	char *first_name;
	char *last_name;
	char *transaction_id;
	char *parent_transaction_id;
	char *transaction_type;
	char *payment_type;
	char *order_time;
	char *amt;
	char *fee_amt;
	char *tax_amt;
	char *currency_code;
	char *payment_status;
	char *pending_reason;
	char *reason_code;
	char *shipping_method;
	struct item *items;
};

int strlen2(char *c) {
	return (c == NULL) ? 0 : strlen(c);
}

void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
	size_t new_len = s->len + size * nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size * nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size * nmemb;
}

void get(char *url, struct fields *f, struct string *s) {
	CURL *curl;
	CURLcode res;
	char tmp[255];

	int i = 0;
	do {
		if (i > 0) {
			f = (*f).next;
		}

		strcat(tmp, (*f).name);
		strcat(tmp, "=");
		strcat(tmp, (*f).value);
		strcat(tmp, "&");
		i++;
	} while ((*f).next);

	printf("%s\n", tmp);

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, strcat(url, tmp));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, s);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

}

char *post(char *url, /*struct fields *f*/char *post_fields) {
	CURL *curl;
	CURLcode res;
	char *error_buffer = NULL;
	struct string *s;
	s = malloc(sizeof(s));
	init_string(s);
	/*
	 char tmp[255];

	 int i = 0;
	 do {
	 if (i > 0) {
	 f = (*f).next;
	 }

	 strcat(tmp, (*f).name);
	 strcat(tmp, "=");
	 strcat(tmp, (*f).value);
	 strcat(tmp, "&");
	 i++;
	 } while ((*f).next);

	 printf("%s\n", tmp);
	 */
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, s);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

		res = curl_easy_perform(curl);
		if (res != 0) {
			printf("%s\n", error_buffer);
		}
		curl_easy_cleanup(curl);
	}

	//printf("%s\n", (*s).ptr);
	return curl_easy_unescape(curl, (*s).ptr, 0, NULL);
}

struct two_array *explode(char *search_string, int size) {
	//search_string = "RECEIVERBUSINESS=paintings.suppliersz@gmail.com&RECEIVEREMAIL=paintings.suppliersz@gmail.com&RECEIVERID=MRBGA4GTHLDH4&EMAIL=kuzz_michael_8807@hotmail.com&PAYERID=K4J3CUQ83JHVW&PAYERSTATUS=verified&COUNTRYCODE=AU&SHIPTONAME=Kristen Bruhn&SHIPTOSTREET=Po Box 1523&SHIPTOCITY=Loxton&SHIPTOSTATE=South Australia&SHIPTOCOUNTRYCODE=AU&SHIPTOCOUNTRYNAME=Australia&SHIPTOZIP=5333&ADDRESSOWNER=PayPal&ADDRESSSTATUS=Unconfirmed&SALESTAX=0.00&BUYERID=kuzzkristen&CLOSINGDATE=2012-02-29T18:16:46Z&TIMESTAMP=2012-03-04T04:35:50Z&CORRELATIONID=5128e2db22988&ACK=Success&VERSION=51.0&BUILD=2571254&FIRSTNAME=Kristen&LASTNAME=Bruhn&TRANSACTIONID=9EE891580X576560R&TRANSACTIONTYPE=cart&PAYMENTTYPE=instant&ORDERTIME=2012-03-01T01:18:22Z&AMT=4.13&FEEAMT=0.30&TAXAMT=0.00&CURRENCYCODE=AUD&PAYMENTSTATUS=Completed&PENDINGREASON=None&REASONCODE=None&SHIPPINGMETHOD=Default&L_NAME0=Large Zebra Foldable Lady Makeup Cosmetic Container Hand Case Pouch Bag + Mirror&L_NUMBER0=140645472198&L_QTY0=1&L_CURRENCYCODE0=AUD&L_AMT0=4.13";
	//search_string = "L_TIMESTAMP0=2012-03-01T04:49:12Z&L_TIMESTAMP1=2012-03-01T03:26:08Z&L_TIMESTAMP2=2012-03-01T03:07:15Z&L_TIMESTAMP3=2012-03-01T03:02:20Z&L_TIMESTAMP4=2012-03-01T02:49:55Z&L_TIMESTAMP5=2012-03-01T02:17:02Z&L_TIMESTAMP6=2012-03-01T02:17:02Z&L_TIMESTAMP7=2012-03-01T02:17:02Z&L_TIMESTAMP8=2012-03-01T01:18:23Z&L_TIMESTAMP9=2012-03-01T01:10:13Z&L_TIMESTAMP10=2012-03-01T00:08:33Z&L_TIMESTAMP11=2012-03-01T00:07:17Z&L_TIMEZONE0=GMT&L_TIMEZONE1=GMT&L_TIMEZONE2=GMT&L_TIMEZONE3=GMT&L_TIMEZONE4=GMT&L_TIMEZONE5=GMT&L_TIMEZONE6=GMT&L_TIMEZONE7=GMT&L_TIMEZONE8=GMT&L_TIMEZONE9=GMT&L_TIMEZONE10=GMT&L_TIMEZONE11=GMT&L_TYPE0=Payment&L_TYPE1=Payment&L_TYPE2=Payment&L_TYPE3=Payment&L_TYPE4=Payment&L_TYPE5=Payment&L_TYPE6=Currency Conversion (credit)&L_TYPE7=Currency Conversion (debit)&L_TYPE8=Payment&L_TYPE9=Payment&L_TYPE10=Payment&L_TYPE11=Payment&L_EMAIL0=heidi_clarke88@hotmail.com&L_EMAIL1=toni_flanagan@hotmail.com&L_EMAIL2=louiskordal@optusnet.com.au&L_EMAIL3=christyloesch@bigpond.com&L_EMAIL4=sarahgrimes@live.com.au&L_EMAIL8=kuzz_michael_8807@hotmail.com&L_EMAIL9=katrena68@hotmail.com&L_EMAIL10=lee59au@bigpond.com&L_EMAIL11=lee59au@bigpond.com&L_NAME0=Heidi L Clarke&L_NAME1=Toni Louise Flanagan&L_NAME2=Louis Kordal&L_NAME3=Christy Loesch&L_NAME4=Sarah Grimes&L_NAME5=eBay International AG&L_NAME6=From Australian Dollar&L_NAME7=To U.S. Dollar&L_NAME8=Kristen Bruhn&L_NAME9=katrena sachnowsky&L_NAME10=Robyn McLennan&L_NAME11=Robyn McLennan&L_TRANSACTIONID0=39Y65362S28792816&L_TRANSACTIONID1=25E072784U996633H&L_TRANSACTIONID2=5UD342976N520160H&L_TRANSACTIONID3=8S924275T0189915G&L_TRANSACTIONID4=3RW716990E156532H&L_TRANSACTIONID5=72774613PC099223M&L_TRANSACTIONID6=93K47502MU590630N&L_TRANSACTIONID7=7BU98985XN1922310&L_TRANSACTIONID8=9EE891580X576560R&L_TRANSACTIONID9=3HT48667TK7709842&L_TRANSACTIONID10=2J7860652S7902314&L_TRANSACTIONID11=88932963F5209964V&L_STATUS0=Completed&L_STATUS1=Completed&L_STATUS2=Completed&L_STATUS3=Completed&L_STATUS4=Completed&L_STATUS5=Completed&L_STATUS6=Completed&L_STATUS7=Completed&L_STATUS8=Completed&L_STATUS9=Completed&L_STATUS10=Completed&L_STATUS11=Completed&L_AMT0=1.99&L_AMT1=17.29&L_AMT2=6.19&L_AMT3=4.38&L_AMT4=4.18&L_AMT5=-800.00&L_AMT6=766.37&L_AMT7=-729.71&L_AMT8=4.13&L_AMT9=4.29&L_AMT10=2.49&L_AMT11=2.49&L_CURRENCYCODE0=AUD&L_CURRENCYCODE1=AUD&L_CURRENCYCODE2=AUD&L_CURRENCYCODE3=AUD&L_CURRENCYCODE4=AUD&L_CURRENCYCODE5=USD&L_CURRENCYCODE6=USD&L_CURRENCYCODE7=AUD&L_CURRENCYCODE8=AUD&L_CURRENCYCODE9=AUD&L_CURRENCYCODE10=AUD&L_CURRENCYCODE11=AUD&L_FEEAMT0=-0.17&L_FEEAMT1=-1.09&L_FEEAMT2=-0.42&L_FEEAMT3=-0.31&L_FEEAMT4=-0.30&L_FEEAMT5=0.00&L_FEEAMT6=0.00&L_FEEAMT7=0.00&L_FEEAMT8=-0.30&L_FEEAMT9=-0.31&L_FEEAMT10=-0.20&L_FEEAMT11=-0.20&L_NETAMT0=1.82&L_NETAMT1=16.20&L_NETAMT2=5.77&L_NETAMT3=4.07&L_NETAMT4=3.88&L_NETAMT5=-800.00&L_NETAMT6=766.37&L_NETAMT7=-729.71&L_NETAMT8=3.83&L_NETAMT9=3.98&L_NETAMT10=2.29&L_NETAMT11=2.29&TIMESTAMP=2012-03-07T11:47:06Z&CORRELATIONID=d1576dec711ec&ACK=Success&VERSION=51.0&BUILD=2653900";
	char *array[size];
	char **data;
	int loop;
	struct two_array *tmp;

	data = malloc(size * sizeof(char *));
	if (data == NULL) {
		printf("out of memory\n");
		exit(1);
	}

	array[0] = strtok(search_string, "&");
	if (array[0] == NULL) {
		printf("No test to search.\n");
		exit(0);
	}

	for (loop = 1; loop < size; loop++) {
		array[loop] = strtok(NULL, "&");
		if (array[loop] == NULL)
			break;
	}

	int i = 0;
	for (loop = 0; loop < size; loop++) {
		if (array[loop] == NULL)
			break;
		data[i] = malloc(strlen(array[loop]) + 1);
		if (data[i] == NULL) {
			printf("out of memory\n");
			exit(1);
		}
		strcpy(data[i], array[loop]);
		printf("Item #%d is %s\n", loop, array[loop]);
		i++;
	}

	tmp = malloc(sizeof(struct two_array));
	tmp->array = data;
	tmp->length = i;
	return tmp;
}

struct transaction *init_transaction() {
	struct transaction *t = malloc(sizeof(struct transaction));
	t->time_stamp = NULL;
	t->time_zone = NULL;
	t->type = NULL;
	t->email = NULL;
	t->name = NULL;
	t->transaction_id = NULL;
	t->status = NULL;
	t->amt = NULL;
	t->currency_code = NULL;
	t->fee_amt = NULL;
	t->net_amt = NULL;
	return t;
}

struct transaction_array *parse_transaction(struct two_array *tmp) {
	//printf("%s|%d\n", tmp->array, tmp->length);
	int i, j, l, count = 0; //count = (tmp->length - 5) / TRA_LEN;
	char *key, *value, *point;
	struct transaction_array *trasaction_array;
	struct transaction *t;

	for (i = 0; i < TRA_PAGE_SIZE; i++) {
		if (strstr(tmp->array[i], "L_TIMESTAMP")) {
			count++;
		} else {
			break;
		}
	}

	printf("transaction count:%d\n", count);

	struct transaction **tra = malloc(sizeof(struct transaction) * count);
	//tra[i] = malloc(sizeof(struct transaction *) * count);
	for (i = 0; i < count; i++) {
		t = init_transaction();
		tra[i] = t;
	}

	for (j = 0; j < tmp->length; j++) {
		key = strtok(tmp->array[j], "=");
		value = strtok(NULL, "=");
		point = tmp->array[j];
		//printf("[%s]%s\n", key, value);
		if (strstr(key, "L_TIMESTAMP")) {
			point += strlen("L_TIMESTAMP");
			l = atoi(point);
			tra[l]->time_stamp = malloc(strlen(value) + 1);
			strcpy(tra[l]->time_stamp, value);
		} else if (strstr(key, "L_TIMEZONE")) {
			point += strlen("L_TIMEZONE");
			l = atoi(point);
			tra[l]->time_zone = malloc(strlen(value) + 1);
			strcpy(tra[l]->time_zone, value);
		} else if (strstr(key, "L_TYPE")) {
			point += strlen("L_TYPE");
			l = atoi(point);
			tra[l]->type = malloc(strlen(value) + 1);
			strcpy(tra[l]->type, value);
		} else if (strstr(key, "L_EMAIL")) {
			point += strlen("L_EMAIL");
			l = atoi(point);
			tra[l]->email = malloc(strlen(value) + 1);
			strcpy(tra[l]->email, value);
		} else if (strstr(key, "L_NAME")) {
			point += strlen("L_NAME");
			l = atoi(point);
			tra[l]->name = malloc(strlen(value) + 1);
			strcpy(tra[l]->name, value);
		} else if (strstr(key, "L_TRANSACTIONID")) {
			point += strlen("L_TRANSACTIONID");
			l = atoi(point);
			tra[l]->transaction_id = malloc(strlen(value) + 1);
			strcpy(tra[l]->transaction_id, value);
		} else if (strstr(key, "L_STATUS")) {
			point += strlen("L_STATUS");
			l = atoi(point);
			tra[l]->status = malloc(strlen(value) + 1);
			strcpy(tra[l]->status, value);
		} else if (strstr(key, "L_AMT")) {
			point += strlen("L_AMT");
			l = atoi(point);
			tra[l]->amt = malloc(strlen(value) + 1);
			strcpy(tra[l]->amt, value);
		} else if (strstr(key, "L_CURRENCYCODE")) {
			point += strlen("L_CURRENCYCODE");
			l = atoi(point);
			tra[l]->currency_code = malloc(strlen(value) + 1);
			strcpy(tra[l]->currency_code, value);
		} else if (strstr(key, "L_FEEAMT")) {
			point += strlen("L_FEEAMT");
			l = atoi(point);
			tra[l]->fee_amt = malloc(strlen(value) + 1);
			strcpy(tra[l]->fee_amt, value);
		} else if (strstr(key, "L_NETAMT")) {
			point += strlen("L_NETAMT");
			l = atoi(point);
			tra[l]->net_amt = malloc(strlen(value) + 1);
			strcpy(tra[l]->net_amt, value);
		}
	}

	trasaction_array = malloc(sizeof(struct transaction_array));
	trasaction_array->tra = tra;
	trasaction_array->length = count;

	return trasaction_array;
}

struct transaction_detail *init_transaction_detail() {
	struct transaction_detail *trad = (struct transaction_detail *) malloc(
			sizeof(struct transaction_detail));
	trad->receiver_business = NULL;
	trad->receiver_email = NULL;
	trad->receiver_id = NULL;
	trad->email = NULL;
	trad->payer_id = NULL;
	trad->payer_status = NULL;
	trad->country_code = NULL;
	trad->ship_to_name = NULL;
	trad->ship_street = NULL;
	trad->ship_to_city = NULL;
	trad->ship_to_state = NULL;
	trad->ship_to_country_code = NULL;
	trad->ship_to_country_name = NULL;
	trad->ship_to_zip = NULL;
	trad->address_owner = NULL;
	trad->address_status = NULL;
	trad->sales_tax = NULL;
	trad->buyer_id = NULL;
	trad->closing_date = NULL;
	trad->time_stamp = NULL;
	trad->correlation_id = NULL;
	trad->ack = NULL;
	trad->version = NULL;
	trad->build = NULL;
	trad->first_name = NULL;
	trad->last_name = NULL;
	trad->transaction_id = NULL;
	trad->transaction_type = NULL;
	trad->payment_type = NULL;
	trad->order_time = NULL;
	trad->amt = NULL;
	trad->fee_amt = NULL;
	trad->tax_amt = NULL;
	trad->currency_code = NULL;
	trad->payment_status = NULL;
	trad->pending_reason = NULL;
	trad->reason_code = NULL;
	trad->shipping_method = NULL;
	return trad;
}

struct transaction_detail *parse_transaction_detail(struct two_array *tmp) {
	int j;
	char *key, *value;
	struct transaction_detail *trad;

	trad = init_transaction_detail();
	for (j = 0; j < tmp->length; j++) {
		key = strtok(tmp->array[j], "=");
		value = strtok(NULL, "=");
		//printf("[%s]=%s\n", key, value);
		if (strcmp(key, "RECEIVERBUSINESS") == 0) {
			trad->receiver_business = malloc(strlen(value) + 1);
			strcpy(trad->receiver_business, value);
		} else if (strcmp(key, "RECEIVEREMAIL") == 0) {
			trad->receiver_email = malloc(strlen(value) + 1);
			strcpy(trad->receiver_email, value);
		} else if (strcmp(key, "RECEIVERID") == 0) {
			trad->receiver_id = malloc(strlen(value) + 1);
			strcpy(trad->receiver_id, value);
		} else if (strcmp(key, "EMAIL") == 0) {
			trad->email = malloc(strlen(value) + 1);
			strcpy(trad->email, value);
		} else if (strcmp(key, "PAYERID") == 0) {
			trad->payer_id = malloc(strlen(value) + 1);
			strcpy(trad->payer_id, value);
		} else if (strcmp(key, "PAYERSTATUS") == 0) {
			trad->payer_status = malloc(strlen(value) + 1);
			strcpy(trad->payer_status, value);
		} else if (strcmp(key, "COUNTRYCODE") == 0) {
			trad->country_code = malloc(strlen(value) + 1);
			strcpy(trad->country_code, value);
		} else if (strcmp(key, "SHIPTONAME") == 0) {
			trad->ship_to_name = malloc(strlen(value) + 1);
			strcpy(trad->ship_to_name, value);
		} else if (strcmp(key, "SHIPTOSTREET") == 0) {
			trad->ship_street = malloc(strlen(value) + 1);
			strcpy(trad->ship_street, value);
		} else if (strcmp(key, "SHIPTOCITY") == 0) {
			trad->ship_to_city = malloc(strlen(value) + 1);
			strcpy(trad->ship_to_city, value);
		} else if (strcmp(key, "SHIPTOSTATE") == 0) {
			trad->ship_to_state = malloc(strlen(value) + 1);
			strcpy(trad->ship_to_state, value);
		} else if (strcmp(key, "SHIPTOCOUNTRYCODE") == 0) {
			trad->ship_to_country_code = malloc(strlen(value) + 1);
			strcpy(trad->ship_to_country_code, value);
		} else if (strcmp(key, "SHIPTOCOUNTRYNAME") == 0) {
			trad->ship_to_country_name = malloc(strlen(value) + 1);
			strcpy(trad->ship_to_country_name, value);
		} else if (strcmp(key, "SHIPTOZIP") == 0) {
			trad->ship_to_zip = malloc(strlen(value) + 1);
			strcpy(trad->ship_to_zip, value);
		} else if (strcmp(key, "ADDRESSOWNER") == 0) {
			trad->address_owner = malloc(strlen(value) + 1);
			strcpy(trad->address_owner, value);
		} else if (strcmp(key, "ADDRESSSTATUS") == 0) {
			trad->address_status = malloc(strlen(value) + 1);
			strcpy(trad->address_status, value);
		} else if (strcmp(key, "SALESTAX") == 0) {
			trad->sales_tax = malloc(strlen(value) + 1);
			strcpy(trad->sales_tax, value);
		} else if (strcmp(key, "BUYERID") == 0) {
			trad->buyer_id = malloc(strlen(value) + 1);
			strcpy(trad->buyer_id, value);
		} else if (strcmp(key, "CLOSINGDATE") == 0) {
			trad->closing_date = malloc(strlen(value) + 1);
			strcpy(trad->closing_date, value);
		} else if (strcmp(key, "TIMESTAMP") == 0) {
			trad->time_stamp = malloc(strlen(value) + 1);
			strcpy(trad->time_stamp, value);
		} else if (strcmp(key, "CORRELATIONID") == 0) {
			trad->correlation_id = malloc(strlen(value) + 1);
			strcpy(trad->correlation_id, value);
		} else if (strcmp(key, "ACK") == 0) {
			trad->ack = malloc(strlen(value) + 1);
			strcpy(trad->ack, value);
		} else if (strcmp(key, "VERSION") == 0) {
			trad->version = malloc(strlen(value) + 1);
			strcpy(trad->version, value);
		} else if (strcmp(key, "BUILD") == 0) {
			trad->build = malloc(strlen(value) + 1);
			strcpy(trad->build, value);
		} else if (strcmp(key, "FIRSTNAME") == 0) {
			trad->first_name = malloc(strlen(value) + 1);
			strcpy(trad->first_name, value);
		} else if (strcmp(key, "LASTNAME") == 0) {
			trad->last_name = malloc(strlen(value) + 1);
			strcpy(trad->last_name, value);
		} else if (strcmp(key, "TRANSACTIONID") == 0) {
			trad->transaction_id = malloc(strlen(value) + 1);
			strcpy(trad->transaction_id, value);
		} else if (strcmp(key, "PARENTTRANSACTIONID") == 0) {
			trad->parent_transaction_id = malloc(strlen(value) + 1);
			strcpy(trad->parent_transaction_id, value);
		} else if (strcmp(key, "TRANSACTIONTYPE") == 0) {
			trad->transaction_type = malloc(strlen(value) + 1);
			strcpy(trad->transaction_type, value);
		} else if (strcmp(key, "PAYMENTTYPE") == 0) {
			trad->payment_type = malloc(strlen(value) + 1);
			strcpy(trad->payment_type, value);
		} else if (strcmp(key, "ORDERTIME") == 0) {
			trad->order_time = malloc(strlen(value) + 1);
			strcpy(trad->order_time, value);
		} else if (strcmp(key, "AMT") == 0) {
			trad->amt = malloc(strlen(value) + 1);
			strcpy(trad->amt, value);
		} else if (strcmp(key, "FEEAMT") == 0) {
			trad->fee_amt = malloc(strlen(value) + 1);
			strcpy(trad->fee_amt, value);
		} else if (strcmp(key, "TAXAMT") == 0) {
			trad->tax_amt = malloc(strlen(value) + 1);
			strcpy(trad->tax_amt, value);
		} else if (strcmp(key, "CURRENCYCODE") == 0) {
			trad->currency_code = malloc(strlen(value) + 1);
			strcpy(trad->currency_code, value);
		} else if (strcmp(key, "PAYMENTSTATUS") == 0) {
			trad->payment_status = malloc(strlen(value) + 1);
			strcpy(trad->payment_status, value);
		} else if (strcmp(key, "PENDINGREASON") == 0) {
			trad->pending_reason = malloc(strlen(value) + 1);
			strcpy(trad->pending_reason, value);
		} else if (strcmp(key, "REASONCODE") == 0) {
			trad->reason_code = malloc(strlen(value) + 1);
			strcpy(trad->reason_code, value);
		} else if (strcmp(key, "SHIPPINGMETHOD") == 0) {
			trad->shipping_method = malloc(strlen(value) + 1);
			strcpy(trad->shipping_method, value);
		}
		key = NULL;
		value = NULL;
	}

	printf("%s|%s|%s|%s|%s|%s|%s\n", trad->receiver_business,
			trad->receiver_email, trad->receiver_id, trad->email,
			trad->payer_id, trad->payer_status, trad->country_code);
	printf("%s|%s|%s|%s|%s|%s|%s\n", trad->ship_to_name, trad->ship_street,
			trad->ship_to_city, trad->ship_to_state, trad->ship_to_country_code,
			trad->ship_to_country_name, trad->ship_to_zip);
	printf("%s|%s|%s|%s|%s|%s|%s\n", trad->address_owner, trad->address_status,
			trad->sales_tax, trad->buyer_id, trad->closing_date,
			trad->time_stamp, trad->correlation_id);
	printf("%s|%s|%s|%s|%s|%s|%s\n", trad->ack, trad->version, trad->build,
			trad->first_name, trad->last_name, trad->transaction_id,
			trad->transaction_type);
	printf("%s|%s|%s|%s|%s|%s|%s\n", trad->payment_type, trad->order_time,
			trad->amt, trad->fee_amt, trad->tax_amt, trad->currency_code,
			trad->payment_status);
	printf("%s|%s|%s\n", trad->pending_reason, trad->reason_code,
			trad->shipping_method);
	return trad;
}

MYSQL *get_mysql_connect() {
	MYSQL *conn;
	conn = mysql_init(NULL);
	if (conn == NULL) {
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	if (mysql_real_connect(conn, "127.0.0.1", "root", "", "testdb", 0, NULL,
			0) == NULL) {
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	return conn;
}

int createTransaction(MYSQL *conn, struct transaction *tra) {
	unsigned long str_length_1;
	unsigned long str_length_2;
	unsigned long str_length_3;
	unsigned long str_length_4;
	unsigned long str_length_5;
	unsigned long str_length_6;
	unsigned long str_length_7;
	unsigned long str_length_8;
	unsigned long str_length_9;
	unsigned long str_length_10;
	unsigned long str_length_11;

	printf("%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n", tra->time_stamp, tra->time_zone,
			tra->type, tra->name, tra->transaction_id, tra->status, tra->amt,
			tra->currency_code, tra->fee_amt, tra->net_amt);

	//return 0;
	MYSQL_STMT *stmt = mysql_stmt_init(conn);
	char *sql = INSERT_TRA;
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
		MYSQL_BIND bind[11];
		memset(bind, 0, sizeof(bind));

		str_length_1 = (tra->time_stamp == NULL) ? 0 : strlen(tra->time_stamp);
		str_length_2 = (tra->time_zone == NULL) ? 0 : strlen(tra->time_zone);
		str_length_3 = (tra->type == NULL) ? 0 : strlen(tra->type);
		str_length_4 = (tra->email == NULL) ? 0 : strlen(tra->email);
		str_length_5 = (tra->name == NULL) ? 0 : strlen(tra->name);
		str_length_6 =
				(tra->transaction_id == NULL) ? 0 : strlen(tra->transaction_id);
		str_length_7 = (tra->status == NULL) ? 0 : strlen(tra->status);
		str_length_8 = (tra->amt == NULL) ? 0 : strlen(tra->amt);
		str_length_9 =
				(tra->currency_code == NULL) ? 0 : strlen(tra->currency_code);
		str_length_10 = (tra->fee_amt == NULL) ? 0 : strlen(tra->fee_amt);
		str_length_11 = (tra->net_amt == NULL) ? 0 : strlen(tra->net_amt);

		bind[0].buffer_type = MYSQL_TYPE_STRING;
		bind[0].buffer = tra->time_stamp;
		bind[0].buffer_length = 20;
		bind[0].is_null = 0;
		bind[0].length = &str_length_1;

		bind[1].buffer_type = MYSQL_TYPE_STRING;
		bind[1].buffer = tra->time_zone;
		bind[1].buffer_length = 5;
		bind[1].is_null = 0;
		bind[1].length = &str_length_2;

		bind[2].buffer_type = MYSQL_TYPE_STRING;
		bind[2].buffer = tra->type;
		bind[2].buffer_length = 5;
		bind[2].is_null = 0;
		bind[2].length = &str_length_3;

		bind[3].buffer_type = MYSQL_TYPE_STRING;
		bind[3].buffer = tra->email;
		bind[3].buffer_length = 50;
		bind[3].is_null = 0;
		bind[3].length = &str_length_4;

		bind[4].buffer_type = MYSQL_TYPE_STRING;
		bind[4].buffer = tra->name;
		bind[4].buffer_length = 50;
		bind[4].is_null = 0;
		bind[4].length = &str_length_5;

		bind[5].buffer_type = MYSQL_TYPE_STRING;
		bind[5].buffer = tra->transaction_id;
		bind[5].buffer_length = 23;
		bind[5].is_null = 0;
		bind[5].length = &str_length_6;

		bind[6].buffer_type = MYSQL_TYPE_STRING;
		bind[6].buffer = tra->status;
		bind[6].buffer_length = 15;
		bind[6].is_null = 0;
		bind[6].length = &str_length_7;

		bind[7].buffer_type = MYSQL_TYPE_STRING;
		bind[7].buffer = tra->amt;
		bind[7].buffer_length = 8;
		bind[7].is_null = 0;
		bind[7].length = &str_length_8;

		bind[8].buffer_type = MYSQL_TYPE_STRING;
		bind[8].buffer = tra->currency_code;
		bind[8].buffer_length = 4;
		bind[8].is_null = 0;
		bind[8].length = &str_length_9;

		bind[9].buffer_type = MYSQL_TYPE_STRING;
		bind[9].buffer = tra->fee_amt;
		bind[9].buffer_length = 8;
		bind[9].is_null = 0;
		bind[9].length = &str_length_10;

		bind[10].buffer_type = MYSQL_TYPE_STRING;
		bind[10].buffer = tra->net_amt;
		bind[10].buffer_length = 8;
		bind[10].is_null = 0;
		bind[10].length = &str_length_11;

		if (mysql_stmt_bind_param(stmt, bind) != 0) {
			printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			exit(1);
		}

		if (mysql_stmt_execute(stmt) != 0) {
			printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			//exit(1);
		}

		if (mysql_stmt_close(stmt) != 0) {
			printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			exit(1);
		}
	} else {
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	return 0;
}

int create_transaction_detail(MYSQL *conn, struct transaction_detail *trad) {
	unsigned long str_length_1;
	unsigned long str_length_2;
	unsigned long str_length_3;
	unsigned long str_length_4;
	unsigned long str_length_5;
	unsigned long str_length_6;
	unsigned long str_length_7;
	unsigned long str_length_8;
	unsigned long str_length_9;
	unsigned long str_length_10;
	unsigned long str_length_11;
	unsigned long str_length_12;
	unsigned long str_length_13;
	unsigned long str_length_14;
	unsigned long str_length_15;
	unsigned long str_length_16;
	unsigned long str_length_17;
	unsigned long str_length_18;
	unsigned long str_length_19;
	unsigned long str_length_20;
	unsigned long str_length_21;
	unsigned long str_length_22;
	unsigned long str_length_23;
	unsigned long str_length_24;
	unsigned long str_length_25;
	unsigned long str_length_26;
	unsigned long str_length_27;
	unsigned long str_length_28;
	unsigned long str_length_29;
	unsigned long str_length_30;
	unsigned long str_length_31;
	unsigned long str_length_32;
	unsigned long str_length_33;
	unsigned long str_length_34;
	unsigned long str_length_35;
	unsigned long str_length_36;
	unsigned long str_length_37;
	unsigned long str_length_38;

	MYSQL_STMT *stmt = mysql_stmt_init(conn);
	char *sql = INSERT_TRA_DETAIL;
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
		MYSQL_BIND bind[38];
		memset(bind, 0, sizeof(bind));

		str_length_1 = strlen2(trad->receiver_business);
		str_length_2 = strlen2(trad->receiver_email);
		str_length_3 = strlen2(trad->receiver_id);
		str_length_4 = strlen2(trad->email);
		str_length_5 = strlen2(trad->payer_id);
		str_length_6 = strlen2(trad->payer_status);
		str_length_7 = strlen2(trad->country_code);
		str_length_8 = strlen2(trad->ship_to_name);
		str_length_9 = strlen2(trad->ship_street);
		str_length_10 = strlen2(trad->ship_to_city);
		str_length_11 = strlen2(trad->ship_to_state);
		str_length_12 = strlen2(trad->ship_to_country_code);
		str_length_13 = strlen2(trad->ship_to_country_name);
		str_length_14 = strlen2(trad->ship_to_zip);
		str_length_15 = strlen2(trad->address_owner);
		str_length_16 = strlen2(trad->address_status);
		str_length_17 = strlen2(trad->sales_tax);
		str_length_18 = strlen2(trad->buyer_id);
		str_length_19 = strlen2(trad->closing_date);
		str_length_20 = strlen2(trad->time_stamp);
		str_length_21 = strlen2(trad->correlation_id);
		str_length_22 = strlen2(trad->ack);
		str_length_23 = strlen2(trad->version);
		str_length_24 = strlen2(trad->build);
		str_length_25 = strlen2(trad->first_name);
		str_length_26 = strlen2(trad->last_name);
		str_length_27 = strlen2(trad->transaction_id);
		str_length_28 = strlen2(trad->transaction_type);
		str_length_29 = strlen2(trad->payment_type);
		str_length_30 = strlen2(trad->order_time);
		str_length_31 = strlen2(trad->amt);
		str_length_32 = strlen2(trad->fee_amt);
		str_length_33 = strlen2(trad->tax_amt);
		str_length_34 = strlen2(trad->currency_code);
		str_length_35 = strlen2(trad->payment_status);
		str_length_36 = strlen2(trad->pending_reason);
		str_length_37 = strlen2(trad->reason_code);
		str_length_38 = strlen2(trad->shipping_method);

		bind[0].buffer_type = MYSQL_TYPE_STRING;
		bind[0].buffer = trad->receiver_business;
		bind[0].buffer_length = 50;
		bind[0].is_null = 0;
		bind[0].length = &str_length_1;

		bind[1].buffer_type = MYSQL_TYPE_STRING;
		bind[1].buffer = trad->receiver_email;
		bind[1].buffer_length = 50;
		bind[1].is_null = 0;
		bind[1].length = &str_length_2;

		bind[2].buffer_type = MYSQL_TYPE_STRING;
		bind[2].buffer = trad->receiver_id;
		bind[2].buffer_length = 50;
		bind[2].is_null = 0;
		bind[2].length = &str_length_3;

		bind[3].buffer_type = MYSQL_TYPE_STRING;
		bind[3].buffer = trad->email;
		bind[3].buffer_length = 50;
		bind[3].is_null = 0;
		bind[3].length = &str_length_4;

		bind[4].buffer_type = MYSQL_TYPE_STRING;
		bind[4].buffer = trad->payer_id;
		bind[4].buffer_length = 50;
		bind[4].is_null = 0;
		bind[4].length = &str_length_5;

		bind[5].buffer_type = MYSQL_TYPE_STRING;
		bind[5].buffer = trad->payer_status;
		bind[5].buffer_length = 15;
		bind[5].is_null = 0;
		bind[5].length = &str_length_6;

		bind[6].buffer_type = MYSQL_TYPE_STRING;
		bind[6].buffer = trad->country_code;
		bind[6].buffer_length = 3;
		bind[6].is_null = 0;
		bind[6].length = &str_length_7;

		bind[7].buffer_type = MYSQL_TYPE_STRING;
		bind[7].buffer = trad->ship_to_name;
		bind[7].buffer_length = 30;
		bind[7].is_null = 0;
		bind[7].length = &str_length_8;

		bind[8].buffer_type = MYSQL_TYPE_STRING;
		bind[8].buffer = trad->ship_street;
		bind[8].buffer_length = 50;
		bind[8].is_null = 0;
		bind[8].length = &str_length_9;

		bind[9].buffer_type = MYSQL_TYPE_STRING;
		bind[9].buffer = trad->ship_to_city;
		bind[9].buffer_length = 50;
		bind[9].is_null = 0;
		bind[9].length = &str_length_10;

		bind[10].buffer_type = MYSQL_TYPE_STRING;
		bind[10].buffer = trad->ship_to_state;
		bind[10].buffer_length = 50;
		bind[10].is_null = 0;
		bind[10].length = &str_length_11;

		bind[11].buffer_type = MYSQL_TYPE_STRING;
		bind[11].buffer = trad->ship_to_country_code;
		bind[11].buffer_length = 3;
		bind[11].is_null = 0;
		bind[11].length = &str_length_12;

		bind[12].buffer_type = MYSQL_TYPE_STRING;
		bind[12].buffer = trad->ship_to_country_name;
		bind[12].buffer_length = 30;
		bind[12].is_null = 0;
		bind[12].length = &str_length_13;

		bind[13].buffer_type = MYSQL_TYPE_STRING;
		bind[13].buffer = trad->ship_to_zip;
		bind[13].buffer_length = 15;
		bind[13].is_null = 0;
		bind[13].length = &str_length_14;

		bind[14].buffer_type = MYSQL_TYPE_STRING;
		bind[14].buffer = trad->address_owner;
		bind[14].buffer_length = 15;
		bind[14].is_null = 0;
		bind[14].length = &str_length_15;

		bind[15].buffer_type = MYSQL_TYPE_STRING;
		bind[15].buffer = trad->address_status;
		bind[15].buffer_length = 15;
		bind[15].is_null = 0;
		bind[15].length = &str_length_16;

		bind[16].buffer_type = MYSQL_TYPE_STRING;
		bind[16].buffer = trad->sales_tax;
		bind[16].buffer_length = 8;
		bind[16].is_null = 0;
		bind[16].length = &str_length_17;

		bind[17].buffer_type = MYSQL_TYPE_STRING;
		bind[17].buffer = trad->buyer_id;
		bind[17].buffer_length = 30;
		bind[17].is_null = 0;
		bind[17].length = &str_length_18;

		bind[18].buffer_type = MYSQL_TYPE_STRING;
		bind[18].buffer = trad->closing_date;
		bind[18].buffer_length = 21;
		bind[18].is_null = 0;
		bind[18].length = &str_length_19;

		bind[19].buffer_type = MYSQL_TYPE_STRING;
		bind[19].buffer = trad->time_stamp;
		bind[19].buffer_length = 21;
		bind[19].is_null = 0;
		bind[19].length = &str_length_20;

		bind[20].buffer_type = MYSQL_TYPE_STRING;
		bind[20].buffer = trad->correlation_id;
		bind[20].buffer_length = 20;
		bind[20].is_null = 0;
		bind[20].length = &str_length_21;

		bind[21].buffer_type = MYSQL_TYPE_STRING;
		bind[21].buffer = trad->ack;
		bind[21].buffer_length = 10;
		bind[21].is_null = 0;
		bind[21].length = &str_length_22;

		bind[22].buffer_type = MYSQL_TYPE_STRING;
		bind[22].buffer = trad->version;
		bind[22].buffer_length = 5;
		bind[22].is_null = 0;
		bind[22].length = &str_length_23;

		bind[23].buffer_type = MYSQL_TYPE_STRING;
		bind[23].buffer = trad->build;
		bind[23].buffer_length = 10;
		bind[23].is_null = 0;
		bind[23].length = &str_length_24;

		bind[24].buffer_type = MYSQL_TYPE_STRING;
		bind[24].buffer = trad->first_name;
		bind[24].buffer_length = 15;
		bind[24].is_null = 0;
		bind[24].length = &str_length_25;

		bind[25].buffer_type = MYSQL_TYPE_STRING;
		bind[25].buffer = trad->last_name;
		bind[25].buffer_length = 15;
		bind[25].is_null = 0;
		bind[25].length = &str_length_26;

		bind[26].buffer_type = MYSQL_TYPE_STRING;
		bind[26].buffer = trad->transaction_id;
		bind[26].buffer_length = 23;
		bind[26].is_null = 0;
		bind[26].length = &str_length_27;

		bind[27].buffer_type = MYSQL_TYPE_STRING;
		bind[27].buffer = trad->transaction_type;
		bind[27].buffer_length = 20;
		bind[27].is_null = 0;
		bind[27].length = &str_length_28;

		bind[28].buffer_type = MYSQL_TYPE_STRING;
		bind[28].buffer = trad->payment_type;
		bind[28].buffer_length = 20;
		bind[28].is_null = 0;
		bind[28].length = &str_length_29;

		bind[29].buffer_type = MYSQL_TYPE_STRING;
		bind[29].buffer = trad->order_time;
		bind[29].buffer_length = 21;
		bind[29].is_null = 0;
		bind[29].length = &str_length_30;

		bind[30].buffer_type = MYSQL_TYPE_STRING;
		bind[30].buffer = trad->amt;
		bind[30].buffer_length = 8;
		bind[30].is_null = 0;
		bind[30].length = &str_length_31;

		bind[31].buffer_type = MYSQL_TYPE_STRING;
		bind[31].buffer = trad->fee_amt;
		bind[31].buffer_length = 8;
		bind[31].is_null = 0;
		bind[31].length = &str_length_32;

		bind[32].buffer_type = MYSQL_TYPE_STRING;
		bind[32].buffer = trad->tax_amt;
		bind[32].buffer_length = 8;
		bind[32].is_null = 0;
		bind[32].length = &str_length_33;

		bind[33].buffer_type = MYSQL_TYPE_STRING;
		bind[33].buffer = trad->currency_code;
		bind[33].buffer_length = 4;
		bind[33].is_null = 0;
		bind[33].length = &str_length_34;

		bind[34].buffer_type = MYSQL_TYPE_STRING;
		bind[34].buffer = trad->payment_status;
		bind[34].buffer_length = 15;
		bind[34].is_null = 0;
		bind[34].length = &str_length_35;

		bind[35].buffer_type = MYSQL_TYPE_STRING;
		bind[35].buffer = trad->pending_reason;
		bind[35].buffer_length = 255;
		bind[35].is_null = 0;
		bind[35].length = &str_length_36;

		bind[36].buffer_type = MYSQL_TYPE_STRING;
		bind[36].buffer = trad->reason_code;
		bind[36].buffer_length = 255;
		bind[36].is_null = 0;
		bind[36].length = &str_length_37;

		bind[37].buffer_type = MYSQL_TYPE_STRING;
		bind[37].buffer = trad->shipping_method;
		bind[37].buffer_length = 30;
		bind[37].is_null = 0;
		bind[37].length = &str_length_38;

		if (mysql_stmt_bind_param(stmt, bind) != 0) {
			printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			exit(1);
		}

		if (mysql_stmt_execute(stmt) != 0) {
			printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			//exit(1);
		}

		if (mysql_stmt_close(stmt) != 0) {
			printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			exit(1);
		}
	} else {
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	return 0;
}

char *search_transaction(struct paypal_info *pi, char *start, char *end) {
	char nvpreq[500];
	printf("search trasaction from %s to %s\n", start, end);
	sprintf(nvpreq, NVPREQTS, "TransactionSearch", NVPVERSION, pi->password,
			pi->user_name, pi->signature, start, end);
	char *result = post(NVPAPI_HOST, nvpreq);
	//printf("%s\n", result);
	return result;
}

char *get_transaction_details(struct paypal_info *pi, char *transaction_id) {
	char nvpreq[500];
	printf("search trasaction detail: %s\n", transaction_id);
	sprintf(nvpreq, NVPREQGTD, "GetTransactionDetails", NVPVERSION,
			pi->password, pi->user_name, pi->signature, transaction_id);
	char *result = post(NVPAPI_HOST, nvpreq);
	//printf("%s\n", result);
	return result;
}

struct paypal_info *init_paypal_info(char *user_name, char *password,
		char *signature) {
	struct paypal_info *pi = malloc(sizeof(struct paypal_info));
	pi->user_name = malloc(strlen(user_name) + 1);
	pi->password = malloc(strlen(password) + 1);
	pi->signature = malloc(strlen(signature) + 1);

	strcpy(pi->user_name, user_name);
	strcpy(pi->password, password);
	strcpy(pi->signature, signature);
	return pi;
}

struct paypal_info_array *load_config() {
	char *user_name;
	char *password;
	char *signature;
	char current_path[255];
	dictionary * ini;
	int i = 0;
	char *j;
	struct paypal_info **paypal_info_array;

	getcwd(current_path, 254);
	printf("current path:%s\n", current_path);

	j = malloc(sizeof(char) * 13);
	paypal_info_array = malloc(sizeof(struct paypal_info) * 20);
	ini = iniparser_load(strcat(current_path, "/paypal.ini"));
	do {
		sprintf(j, "%d", i);
		user_name = iniparser_getstring(ini, strcat(j, ":Username"), NULL);
		sprintf(j, "%d", i);
		password = iniparser_getstring(ini, strcat(j, ":Password"), NULL);
		sprintf(j, "%d", i);
		signature = iniparser_getstring(ini, strcat(j, ":Signature"), NULL);
		//printf("%s|%s|%s\n", user_name, password, signature);
		if (user_name == NULL) {
			struct paypal_info_array *tmp = malloc(
					sizeof(struct paypal_info_array));
			tmp->array = paypal_info_array;
			tmp->length = i;
			return tmp;
		}
		paypal_info_array[i] = init_paypal_info(user_name, password, signature);
		i++;
	} while (user_name != NULL);
	return NULL;
}

char *getDateTime(int hours) {
	time_t now = time(NULL);
	time_t time = now + (-8 + hours) * 60 * 60;
	char *result = malloc(sizeof(char) * 20);
	strftime(result, 20, "%Y-%m-%d %H:%M:%S", localtime(&time));
	return result;
}

void run(MYSQL *conn, struct paypal_info *pi) {
	char *transaction_string;
	char *transaction_des_string;
	struct two_array *transaction_o_array;
	struct transaction_array *transaction_array;
	struct transaction_detail *trad;
	struct two_array * trad_o;
	int i;

	char *start = getDateTime(-3);
	char *end = getDateTime(0);

	transaction_string = search_transaction(pi, start, end);
	free(start);
	free(end);
	transaction_o_array = explode(transaction_string, TRA_SIZE);
	transaction_array = parse_transaction(transaction_o_array);
	//exit(1);
	for (i = 0; i < transaction_array->length; i++) {
		//printf("%s\n", transaction_array->tra[i]->time_stamp);
		if (strcmp(transaction_array->tra[i]->type, "Payment") != 0) {
			continue;
		}
		createTransaction(conn, transaction_array->tra[i]);

		transaction_des_string = get_transaction_details(pi,
				transaction_array->tra[i]->transaction_id);
		trad_o = explode(transaction_des_string, TRA_DETAIL_SIZE);
		trad = parse_transaction_detail(trad_o);
		create_transaction_detail(conn, trad);
		free(transaction_des_string);
		free(trad_o);
		free(trad);
	}

	free(transaction_string);
	//free(transaction_o_array->array);
	free(transaction_o_array);
	//free(transaction_array->tra);
	free(transaction_array);

}

int main(void) {
	MYSQL *conn;
	int i;
	conn = get_mysql_connect();

	struct paypal_info_array *paypal_info_array =
			(struct paypal_info_array *) load_config();
	for (i = 0; i < paypal_info_array->length; i++) {
		printf("%s\n", paypal_info_array->array[i]->user_name);
		run(conn, paypal_info_array->array[i]);
		//exit(1);
	}
	mysql_close(conn);

	log4c_category_t* mycat = NULL;
	log4c_init();
	mycat = log4c_category_get("six13log.log.app.application1");
	log4c_category_log(mycat, LOG4C_PRIORITY_DEBUG, "Debugging app 1");
	log4c_fini();
	//get_transaction_details("9EE891580X576560R");
	//json_object *new_obj;
	//new_obj = json_tokener_parse("{ \"abc\": 12, \"foo\": \"bar\", \"bool0\": false, \"bool1\": true, \"arr\": [ 1, 2, 3, null, 5 ] }");
	//printf("new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));
	return 0;
}

