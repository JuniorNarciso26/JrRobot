#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/core/jr_wifi.c"

static unsigned profiles,added,cleared;
static uint8_t pending,migrated;
static bool stored,setup;
static wifi_settings_t legacy;
static esp_wifi_service_t fake_service;
const char *esp_err_to_name(esp_err_t e){(void)e;return "mock";}
esp_err_t nvs_flash_init(void){return ESP_OK;}
esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *h){(void)ns;(void)mode;*h=1;return ESP_OK;}
esp_err_t nvs_get_str(nvs_handle_t h,const char *key,char *v,size_t *n){(void)h;(void)key;(void)v;(void)n;return ESP_ERR_NVS_NOT_FOUND;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *v,size_t *n){(void)h;(void)key;if(!stored)return ESP_ERR_NVS_NOT_FOUND;assert(*n>=sizeof(legacy));memcpy(v,&legacy,sizeof(legacy));*n=sizeof(legacy);return ESP_OK;}
esp_err_t nvs_get_u8(nvs_handle_t h,const char *key,uint8_t *v){(void)h;*v=!strcmp(key,"import_pending")?pending:migrated;return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *v,size_t n){(void)h;(void)key;assert(n==sizeof(legacy));memcpy(&legacy,v,n);stored=true;return ESP_OK;}
esp_err_t nvs_set_u8(nvs_handle_t h,const char *key,uint8_t v){(void)h;if(!strcmp(key,"import_pending"))pending=v;else if(!strcmp(key,"migration_done"))migrated=v;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return ESP_OK;}
void nvs_close(nvs_handle_t h){(void)h;}
esp_err_t esp_wifi_get_config(int i,wifi_config_t *c){(void)i;memcpy(c->sta.ssid,"Home",4);return ESP_OK;}
esp_err_t esp_config_storage_init_nvs(esp_config_storage_nvs_t *s,esp_config_storage_t *out){assert(!strcmp(s->nvs_namespace,"wifi_svc_ex"));*out=s;return ESP_OK;}
esp_err_t esp_wifi_service_profile_mgr_init(const esp_wifi_service_profile_mgr_cfg_t *cfg,esp_wifi_service_profile_mgr_t *out){assert(cfg->max_profiles==8);*out=&profiles;return ESP_OK;}
esp_err_t esp_wifi_service_profile_mgr_count(esp_wifi_service_profile_mgr_t h,uint8_t *n){(void)h;*n=(uint8_t)profiles;return ESP_OK;}
esp_err_t esp_wifi_service_profile_mgr_add(esp_wifi_service_profile_mgr_t h,const esp_wifi_service_profile_t *p){(void)h;assert(!strcmp(p->ssid,"Legacy"));assert(!strcmp(p->password,"password123"));added++;profiles=1;return ESP_OK;}
esp_err_t esp_wifi_service_profile_mgr_clear_all(esp_wifi_service_profile_mgr_t h){(void)h;cleared++;profiles=0;return ESP_OK;}
esp_err_t esp_wifi_service_create(const esp_wifi_service_config_t *cfg,esp_wifi_service_t **out){assert(cfg->prov_num==1);*out=&fake_service;return ESP_OK;}
esp_err_t esp_wifi_service_is_provisioning_running(esp_wifi_service_t *s,bool *r){(void)s;*r=setup;return ESP_OK;}
esp_err_t esp_wifi_service_start_provisioning(esp_wifi_service_t *s){(void)s;setup=true;return ESP_OK;}
esp_err_t esp_wifi_service_prov_http_create(const esp_wifi_service_prov_http_config_t *cfg,esp_wifi_service_prov_t *p){assert(cfg->port==8080);assert(!strcmp(cfg->softap.ssid,"JrBot-Config"));*p=&profiles;return ESP_OK;}
esp_err_t esp_service_event_subscribe(esp_service_t *s,const adf_event_subscribe_info_t *sub){(void)s;assert(sub->handler);return ESP_OK;}
esp_err_t esp_service_start(esp_service_t *s){(void)s;setup=profiles==0;return ESP_OK;}
esp_err_t jr_mdns_start(void){return ESP_OK;}
void esp_fill_random(void *p,size_t n){memset(p,0,n);}
void esp_aes_init(esp_aes_context *p){*p=0;}
void esp_aes_free(esp_aes_context *p){(void)p;}
int esp_aes_setkey(esp_aes_context *p,const uint8_t *k,unsigned n){(void)p;(void)k;(void)n;return 0;}
int esp_aes_crypt_cbc(esp_aes_context *p,int mode,size_t n,uint8_t *iv,const uint8_t *in,uint8_t *out){(void)p;(void)mode;(void)iv;memmove(out,in,n);return 0;}
int esp_aes_crypt_ecb(esp_aes_context *p,int mode,const uint8_t *in,uint8_t *out){(void)p;(void)mode;memmove(out,in,16);return 0;}

int main(int argc,char **argv){
 assert(argc==2);
 defaults(&legacy);copy_checked(legacy.ssid,sizeof(legacy.ssid),"Legacy");copy_checked(legacy.pass,sizeof(legacy.pass),"password123");
 if(!strcmp(argv[1],"serial_save")){stored=false;}
 else if(!strcmp(argv[1],"serial_clear")){stored=false;profiles=1;}
 else if(!strcmp(argv[1],"fresh")){stored=false;}
 else if(!strcmp(argv[1],"legacy")){stored=true;}
 else if(!strcmp(argv[1],"static_migration")){
   stored=true;legacy.use_static=1;
   copy_checked(legacy.ip,sizeof(legacy.ip),"192.168.1.36");
   copy_checked(legacy.gateway,sizeof(legacy.gateway),"192.168.1.1");
   copy_checked(legacy.host,sizeof(legacy.host),"old-robot");
 }
 else if(!strcmp(argv[1],"experiment")){stored=true;profiles=1;}
 else if(!strcmp(argv[1],"deleted")){stored=true;migrated=1;}
 else if(!strcmp(argv[1],"pending_config")){stored=true;pending=1;migrated=1;}
 else if(!strcmp(argv[1],"pending_clear")){stored=true;legacy.ssid[0]=0;pending=1;migrated=1;profiles=1;}
 else return 2;
 jr_wifi_start();
 assert(jr_wifi_network_ready());
 if(!strcmp(argv[1],"static_migration")){
   assert(stored && legacy.use_static==0 && !strcmp(legacy.host,"jrbot"));
   assert(!strcmp(legacy.ssid,"Legacy") && !strcmp(legacy.pass,"password123"));
   assert(pending==0 && added==1);
   puts("PASS static IP metadata migrated to DHCP; credentials retained");
   return 0;
 }
 if(!strcmp(argv[1],"serial_save")||!strcmp(argv[1],"serial_clear")){
   char response[256];
   bool save=!strcmp(argv[1],"serial_save");
   bool ok=save?jr_wifi_configure("Legacy","password123","jrbot",false,NULL,NULL,NULL,NULL,NULL,response,sizeof(response)):jr_wifi_clear(response,sizeof(response));
   assert(ok && pending==1 && state.pending_restart);
   prepared=false;start_attempted=false;
   jr_wifi_start();
   assert(pending==0 && !state.pending_restart);
   if(save)assert(added==1 && profiles==1);
   else assert(cleared==1 && profiles==0);
   printf("PASS %s actual serial persistence/restart (mock backend only)\\n",argv[1]);
   return 0;
 }
 assert(migrated==1);
 if(!strcmp(argv[1],"legacy")||!strcmp(argv[1],"pending_config"))assert(added==1 && profiles==1 && !pending);
 else assert(added==0);
 if(!strcmp(argv[1],"pending_clear"))assert(cleared==1 && profiles==0 && !pending);
 else assert(cleared==0);
 if(!strcmp(argv[1],"deleted")||!strcmp(argv[1],"fresh"))assert(profiles==0 && setup);
 ip_event_got_ip_t ip={.ip_info.ip.addr=0x0401a8c0};
 adf_event_t event={.event_id=ESP_WIFI_SERVICE_EVENT_STA_GOT_IP,.payload=&ip,.payload_len=sizeof(ip)};
 wifi_service_event(&event,NULL);
 assert(jr_wifi_is_connected() && !strcmp(jr_wifi_ip(),"192.168.1.4"));
 event.event_id=ESP_WIFI_SERVICE_EVENT_DISCONNECTED;event.payload=NULL;event.payload_len=0;
 wifi_service_event(&event,NULL);
 assert(!jr_wifi_is_connected() && jr_wifi_network_ready());
 printf("PASS %s migration/status (mock backend only)\n",argv[1]);
 return 0;
}

esp_err_t esp_wifi_service_stop_provisioning(esp_wifi_service_t *s){(void)s;setup=false;return ESP_OK;}
esp_err_t esp_wifi_service_prov_http_apply_credentials(esp_wifi_service_prov_t p,const char *s,const char *password,uint8_t prio){(void)p;(void)s;(void)password;(void)prio;return ESP_OK;}

size_t heap_caps_get_free_size(unsigned c){(void)c;return 1024;}
size_t heap_caps_get_largest_free_block(unsigned c){(void)c;return 512;}
