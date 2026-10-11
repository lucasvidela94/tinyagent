/* TinyShell Agent for Windows 7 x86. Build with a Windows C compiler.
 * Uses WinHTTP (Windows) + OpenCode Go Chat Completions; no runtime dependencies.
 */
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#pragma comment(lib, "winhttp.lib")

#define API_HOST L"opencode.ai"
#define API_PATH L"/zen/go/v1/chat/completions"
#define MODEL "deepseek-v4.1-flash"
#define MAX_HTTP (8 * 1024 * 1024)
#define MAX_TOKENS 20000

typedef struct { int start, end, size, parent; } Tok;
static Tok toks[MAX_TOKENS]; static int ntok;
static char *resp;
static const char *json_src;
static char *history;
static size_t hist_len, hist_cap;

static void die(const char *s) { fprintf(stderr, "\nERROR: %s\n", s); }
static void *xrealloc(void *p, size_t n) { void *q = realloc(p,n); if(!q){die("Memoria insuficiente"); exit(2);} return q; }
static void append(char **buf,size_t *len,size_t *cap,const char *s,size_t n) {
  if(*len+n+1>*cap){size_t c=*cap?*cap:4096; while(c<*len+n+1)c*=2; *buf=(char*)xrealloc(*buf,c);*cap=c;}
  memcpy(*buf+*len,s,n);*len+=n;(*buf)[*len]=0;
}
static void hist(const char *s) { append(&history,&hist_len,&hist_cap,s,strlen(s)); }
static char *jesc(const char *s) {
  size_t n=0,cap=strlen(s)*2+16; char *o=(char*)malloc(cap); if(!o)return NULL;
  for(;*s;s++){ unsigned char c=(unsigned char)*s; char tmp[8]; const char *p=tmp; size_t z=1;
    if(c=='"'||c=='\\'){tmp[0]='\\';tmp[1]=(char)c;z=2;}
    else if(c=='\n'){p="\\n";z=2;} else if(c=='\r'){p="\\r";z=2;} else if(c=='\t'){p="\\t";z=2;}
    else if(c<32){sprintf(tmp,"\\u%04x",c);z=6;}
    else tmp[0]=(char)c;
    if(n+z+1>cap){cap=(cap+z+64)*2;o=(char*)xrealloc(o,cap);} memcpy(o+n,p,z);n+=z;
  } o[n]=0; return o;
}
/* Small JSON tokenizer. Tokens retain source offsets; object keys and values are siblings. */
static int parse_json(const char *s) {
  json_src=s;
  int stack[MAX_TOKENS], sp=0; ntok=0;
  for(int i=0;s[i];){ unsigned char c=(unsigned char)s[i];
    if(isspace(c)||c==','||c==':'){i++;continue;}
    if(c=='{'||c=='['){ if(ntok>=MAX_TOKENS)return 0; int t=ntok++; toks[t]=(Tok){i,-1,0,sp?stack[sp-1]:-1}; if(sp)toks[stack[sp-1]].size++; stack[sp++]=t; i++; continue; }
    if(c=='}'||c==']'){if(!sp)return 0; toks[stack[--sp]].end=i+1;i++;continue;}
    if(c=='"'){int st=++i; while(s[i]){if(s[i]=='\\'&&s[i+1]){i+=2;continue;}if(s[i]=='"')break;i++;} if(!s[i])return 0; if(ntok>=MAX_TOKENS)return 0; int t=ntok++; toks[t]=(Tok){st-1,i+1,0,sp?stack[sp-1]:-1}; if(sp)toks[stack[sp-1]].size++; i++; continue;}
    int st=i; while(s[i]&&!isspace((unsigned char)s[i])&&s[i]!=','&&s[i]!=']'&&s[i]!='}')i++; if(i==st)return 0;
    if(ntok>=MAX_TOKENS)return 0; int t=ntok++; toks[t]=(Tok){st,i,0,sp?stack[sp-1]:-1}; if(sp)toks[stack[sp-1]].size++;
  }
  return sp==0;
}
static int nexttok(int i){int e=toks[i].end; i++; while(i<ntok&&toks[i].start<e)i++;return i;}
static int eqtok(int i,const char *s){if(i<0||i>=ntok)return 0;int a=toks[i].start,b=toks[i].end;if(json_src[a]=='"'){a++;b--;}return (int)strlen(s)==b-a&&!strncmp(json_src+a,s,b-a);}
static int key(int obj,const char *name){if(obj<0)return -1;int end=nexttok(obj);for(int i=obj+1;i<end;){int v=i+1;if(eqtok(i,name))return v;i=nexttok(v);}return -1;}
static char *tokstr(int i){
  if(i<0)return _strdup(""); int a=toks[i].start,b=toks[i].end; if(json_src[a]=='"'){a++;b--;}
  char *o=(char*)malloc((size_t)(b-a)+1); if(!o)return _strdup(""); size_t n=0;
  for(int p=a;p<b;p++){char c=json_src[p];if(c=='\\'&&p+1<b){char d=json_src[++p];switch(d){case 'n':o[n++]='\n';break;case 'r':o[n++]='\r';break;case 't':o[n++]='\t';break;case 'b':o[n++]='\b';break;case 'f':o[n++]='\f';break;case '"':o[n++]='"';break;case '\\':o[n++]='\\';break;case '/':o[n++]='/';break;default:o[n++]=d;break;}}else o[n++]=c;}o[n]=0;return o;
}
static char *slice(int i){if(i<0)return _strdup("null");int a=toks[i].start,b=toks[i].end;char *o=(char*)malloc((size_t)(b-a)+1);memcpy(o,json_src+a,b-a);o[b-a]=0;return o;}
static char *read_all(HINTERNET req){char *b=NULL;size_t n=0,c=0;DWORD avail=0,got=0;do{avail=0;if(!WinHttpQueryDataAvailable(req,&avail))break;if(!avail)break;char *tmp=(char*)malloc(avail+1);if(!tmp)break;if(!WinHttpReadData(req,tmp,avail,&got)){free(tmp);break;}append(&b,&n,&c,tmp,got);free(tmp);}while(avail);if(!b)b=_strdup("");return b;}
static char *http_post(const char *body,const char *api_key){HINTERNET ses=WinHttpOpen(L"TinyShell/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!ses)return NULL;WinHttpSetTimeouts(ses,15000,15000,30000,120000);HINTERNET con=WinHttpConnect(ses,API_HOST,INTERNET_DEFAULT_HTTPS_PORT,0);if(!con){WinHttpCloseHandle(ses);return NULL;}HINTERNET req=WinHttpOpenRequest(con,L"POST",API_PATH,NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!req){WinHttpCloseHandle(con);WinHttpCloseHandle(ses);return NULL;}
  char auth[2048];snprintf(auth,sizeof(auth),"Authorization: Bearer %s\r\nContent-Type: application/json\r\n",api_key);wchar_t wh[4096];MultiByteToWideChar(CP_UTF8,0,auth,-1,wh,4096);
  BOOL ok=WinHttpSendRequest(req,wh,(DWORD)-1,(LPVOID)body,(DWORD)strlen(body),(DWORD)strlen(body),0)&&WinHttpReceiveResponse(req,NULL);char *out=NULL;if(ok){DWORD status=0,sz=sizeof(status);WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&sz,WINHTTP_NO_HEADER_INDEX);out=read_all(req);if(status<200||status>=300){fprintf(stderr,"\nHTTP %lu: %s\n",(unsigned long)status,out);free(out);out=NULL;}}
  WinHttpCloseHandle(req);WinHttpCloseHandle(con);WinHttpCloseHandle(ses);return out;}
static char *build_request(void){size_t cap=strlen(history)+4096;char *b=(char*)malloc(cap);snprintf(b,cap,"{\"model\":\"%s\",\"messages\":%s],\"temperature\":0.1,\"max_tokens\":4096,\"tools\":[{\"type\":\"function\",\"function\":{\"name\":\"shell\",\"description\":\"Run a Windows CMD command on the local machine. Use for diagnostics, inspect files, and repairs.\",\"parameters\":{\"type\":\"object\",\"properties\":{\"command\":{\"type\":\"string\",\"description\":\"Exact command to run in cmd.exe\"},\"reason\":{\"type\":\"string\",\"description\":\"Why this command is needed\"}},\"required\":[\"command\",\"reason\"],\"additionalProperties\":false}}}],\"tool_choice\":\"auto\"}",MODEL,history);return b;}
static int dangerous(const char *s){char l[4096];size_t i;for(i=0;s[i]&&i<sizeof(l)-1;i++)l[i]=(char)tolower((unsigned char)s[i]);l[i]=0;const char *bad[]={"format ","diskpart","del /s","del /q","erase ","rmdir /s","rd /s","reg delete","bcdedit","shutdown","\b",NULL};for(i=0;bad[i];i++)if(strcmp(bad[i],"\b")&&strstr(l,bad[i]))return 1;return 0;}
static char *run_shell(const char *cmd){char line[8192];snprintf(line,sizeof(line),"cmd.exe /d /s /c \"%s 2>&1\"",cmd);FILE *p=_popen(line,"r");if(!p)return _strdup("No se pudo iniciar cmd.exe");char *out=NULL;size_t n=0,c=0;char buf[512];while(fgets(buf,sizeof(buf),p)){if(n>24000)break;append(&out,&n,&c,buf,strlen(buf));}int rc=_pclose(p);if(!out)out=_strdup("(sin salida)");char tail[64];snprintf(tail,sizeof(tail),"\n[exit=%d]",rc);append(&out,&n,&c,tail,strlen(tail));return out;}
static void add_tool_result(const char *id,const char *output){char *ei=jesc(id),*eo=jesc(output);size_t n=strlen(ei)+strlen(eo)+128;char *m=(char*)malloc(n);snprintf(m,n,"%s{\"role\":\"tool\",\"tool_call_id\":\"%s\",\"content\":\"%s\"}",hist_len?",":"",ei,eo);hist(m);free(m);free(ei);free(eo);}
static void trim_newline(char *s){size_t n=strlen(s);while(n&&(s[n-1]=='\n'||s[n-1]=='\r'))s[--n]=0;}
int main(void){SetConsoleOutputCP(CP_UTF8);SetConsoleCP(CP_UTF8);printf("TinyShell Agent | Windows 7 x86 | OpenCode Go / %s\n",MODEL);printf("El modelo es remoto; esta PC solo ejecuta comandos aprobados.\n\nAPI key de OpenCode Go: ");char api[1024];if(!fgets(api,sizeof(api),stdin))return 1;trim_newline(api);if(!*api){puts("Falta API key.");return 1;}
  const char *sys="Sos un agente técnico para diagnosticar y reparar Windows 7 de 32 bits, Atom N2600 y 2 GB RAM. Respondé en español, breve y práctico. Tenés la herramienta shell para ejecutar comandos CMD en la máquina real. Primero inspeccioná y diagnosticá; preferí comandos de lectura como systeminfo, tasklist, wmic, ipconfig, sc query, dir, type, chkdsk sin /f. Ejecutá una sola acción por vez. Para modificar, instalar, borrar, cambiar registro, servicios o configuración, explicá el riesgo y pedí aprobación en el chat antes de llamar shell. Nunca borres datos, formatees discos ni desactives antivirus/firewall. No supongas que una reparación funcionó: verificá el resultado.";
  char *es=jesc(sys);size_t sn=strlen(es)+64;char *sm=(char*)malloc(sn);snprintf(sm,sn,"[{\"role\":\"system\",\"content\":\"%s\"}",es);history=sm;hist_len=strlen(sm);hist_cap=sn;free(es);
  char input[4096];for(;;){printf("\nvos> ");if(!fgets(input,sizeof(input),stdin))break;trim_newline(input);if(!strcmp(input,"/salir")||!strcmp(input,"/exit"))break;if(!*input)continue;char *ei=jesc(input);size_t mn=strlen(ei)+64;char *m=(char*)malloc(mn);snprintf(m,mn,",{\"role\":\"user\",\"content\":\"%s\"}",ei);hist(m);free(m);free(ei);
    int turns=0;while(turns++<8){char *body=build_request();char *r=http_post(body,api);free(body);if(!r){puts("\nNo se pudo contactar OpenCode Go. Revisá conexión, API key y fecha/hora de Windows.");break;}resp=r;if(!parse_json(resp)){puts("\nRespuesta JSON inválida.");free(resp);resp=NULL;break;}int choices=key(0,"choices");int choice=choices>=0?choices+1:-1;int msg=choice>=0?key(choice,"message"):-1;if(msg<0){puts("\nRespuesta sin message.");free(resp);resp=NULL;break;}int tc=key(msg,"tool_calls");if(tc>=0&&toks[tc].size>0){char *raw=slice(msg);size_t z=strlen(raw)+80;char *am=(char*)malloc(z);snprintf(am,z,",%s",raw);hist(am);free(am);free(raw);
        int end=nexttok(tc);for(int ti=tc+1;ti<end;ti=nexttok(ti)){int idt=key(ti,"id"),fn=key(ti,"function"),nam=key(fn,"name"),arg=key(fn,"arguments");char *id=tokstr(idt),*name=tokstr(nam),*args=tokstr(arg);char *output=NULL;
          if(strcmp(name,"shell")){output=_strdup("Herramienta no permitida.");}
          else {char *cmd=NULL; char *reason=NULL; Tok *saved=(Tok*)malloc((size_t)ntok*sizeof(Tok));int saved_n=ntok;const char *saved_src=json_src;memcpy(saved,toks,(size_t)ntok*sizeof(Tok));
            if(parse_json(args)){int root=0,ct=key(root,"command"),rt=key(root,"reason");cmd=tokstr(ct);reason=tokstr(rt);}else {cmd=_strdup("");reason=_strdup("");}
            memcpy(toks,saved,(size_t)saved_n*sizeof(Tok));ntok=saved_n;json_src=saved_src;free(saved);printf("\n[PROPUESTA] %s\nMotivo: %s\n",cmd,reason);free(reason);
            if(!*cmd)output=_strdup("Faltó el comando.");else {printf("Ejecutar? [s/N]: ");char yn[32];if(!fgets(yn,sizeof(yn),stdin)||tolower((unsigned char)yn[0])!='s')output=_strdup("Usuario rechazó la ejecución. No se ejecutó nada.");else if(dangerous(cmd)){printf("Comando potencialmente destructivo. Confirmá escribiendo EXACTAMENTE EJECUTAR: ");char confirm[32];if(!fgets(confirm,sizeof(confirm),stdin)){output=_strdup("Cancelado.");}else{trim_newline(confirm);if(strcmp(confirm,"EJECUTAR"))output=_strdup("Cancelado por seguridad.");else output=run_shell(cmd);}}else output=run_shell(cmd);}free(cmd);}
          add_tool_result(id,output?output:"(sin salida)");free(output);free(id);free(name);free(args);
        }free(resp);resp=NULL;continue;
      }
      int content=key(msg,"content");char *answer=tokstr(content);printf("\n%s\n",answer);char *ea=jesc(answer);size_t az=strlen(ea)+64;char *am=(char*)malloc(az);snprintf(am,az,",{\"role\":\"assistant\",\"content\":\"%s\"}",ea);hist(am);free(am);free(ea);free(answer);free(resp);resp=NULL;break;
    }
  }
  free(history);puts("\nSaliendo.");return 0;
}
