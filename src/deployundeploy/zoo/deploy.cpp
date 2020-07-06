
#include "httpfuntions.hpp"
#include "xmlmemorywritewrapper.hpp"
#include <cstdio>
#include <eoepca/owl/eoepcaows.hpp>
#include <eoepca/owl/owsparameter.hpp>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <zooconverter.hpp>

// LINUX MKDIR
#include <sys/stat.h>
#include <sys/types.h>
// LINUX MKDIR

#include "service.h"
#include "service_internal.h"

int mkpath(char *file_path, mode_t mode) {
  // assert(file_path && *file_path);
  if (file_path!=nullptr)
    for (char *p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
      *p = '\0';
      if (mkdir(file_path, mode) == -1) {
        if (errno != EEXIST) {
          *p = '/';
          return -1;
        }
      }
      *p = '/';
    }
  return 0;
}

void getT2ConfigurationFromZooMapConfig(
    maps *&conf, std::string what,
    std::map<std::string, std::string> &configs) {
  maps *t2wps = getMaps(conf, what.c_str());
  if (t2wps && t2wps->content) {
    map *tmp = t2wps->content;
    while (tmp != NULL) {
      configs[tmp->name] = tmp->value;
      tmp = tmp->next;
    }
  }
}

int removeFile(const char *filename) {
  if (!filename)
    return 0;
  return std::remove(filename);
}

bool fileExist(const char *fileName) {
  std::ifstream infile(fileName);
  return infile.good();
}

bool iequals(const std::string &a, const std::string &b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                    [](char a, char b) { return tolower(a) == tolower(b); });
}

int writeContent(const char *fileName, std::string_view content) {
  std::ofstream file(fileName);
  if (file.good()) {
    file << content;
    file.flush();
    file.close();
  } else {
    return 1;
  }

  return 0;
}

int setZooError(maps *conf, std::string_view message, std::string_view code) {
  setMapInMaps(conf, "lenv", "message", message.data());
  setMapInMaps(conf, "lenv", "code", code.data());

  return SERVICE_FAILED;
}

#define LOGTEST (std::cerr)
void _MEdumpMap(map *t) {
  if (t != NULL) {
    LOGTEST << t->name << ": (" << t->value << ")\n";
    //        if(t->next!=NULL){
    //            // _MEdumpMap(t->next);
    //        }
  } else {
    LOGTEST << "NULL\n";
  }
}

void MEdumpMap(map *t) {
  map *tmp = t;
  while (tmp != NULL) {
    _MEdumpMap(tmp);
    tmp = tmp->next;
  }
  LOGTEST << "----------------END\n";
}

void MEdumpMaps(maps *m) {
  maps *tmp = m;
  while (tmp != NULL) {
    LOGTEST << "\n----------------INI\n";
    LOGTEST << "MAP => [" << tmp->name << "] \n";
    LOGTEST << " * CONTENT [" << tmp->name << "] \n";
    LOGTEST << "----------------VALUE" << std::endl;
    MEdumpMap(tmp->content);
    LOGTEST << " * CHILD [" << tmp->name << "] \n" << std::endl;
    MEdumpMaps(tmp->child);
    tmp = tmp->next;
  }
}

enum class Operation { DEPLOY, UNDEPLOY };
enum class DeployResults { NONE, EXIST, CANTWRITE };

//  std::list<std::unique_ptr<ZOO::Zoo>> zooApplicationOk{};
//  std::list<std::unique_ptr<ZOO::Zoo>> zooApplicationNOk{};

DeployResults deploy(std::string_view path, std::string_view content) {
  if (fileExist(path.data())) {
    return DeployResults::EXIST;
  } else {
    if (writeContent(path.data(), content.data())) {
      return DeployResults::CANTWRITE;
    }
  }

  return DeployResults::NONE;
}

void setStatus(maps *&conf, const char *status, const char *message) {

  map *usid = getMapFromMaps(conf, "lenv", "uusid");
  map *r_inputs = NULL;
  r_inputs = getMapFromMaps(conf, "main", "tmpPath");
  char *flenv = (char *)malloc(
      (strlen(r_inputs->value) + strlen(usid->value) + 12) * sizeof(char));
  sprintf(flenv, "%s/%s_lenv.cfg", r_inputs->value, usid->value);
  setMapInMaps(conf, "lenv", "message", message);
  setMapInMaps(conf, "lenv", "status", status);
  maps *lenvMaps = getMaps(conf, "lenv");
  dumpMapsToFile(lenvMaps, flenv, 0);
  free(flenv);
}

int simpleRemove(std::string finalPath, std::string_view owsOri, maps *&conf,
                 maps *&inputs, maps *&outputs) {
  std::stringbuf strBuffer;
  auto xml = std::make_unique<XmlWriteMemoryWrapper>();
  xml->startDocument("", "", "");
  xml->startElement("result");
  xml->writeAttribute("applicationPackageFile", "");

  xml->startElement("operations");
  {
    xml->writeAttribute("packageId", "");

    xml->startElement("operation");
    {
      xml->writeAttribute("processId", "");
      xml->writeAttribute("processVersion", "");
      xml->writeAttribute("version", "");
      xml->writeAttribute("type", "undeploy");

      xml->startElement("wpsId");
      { xml->writeContent(owsOri.data()); }
      xml->endElement();

      xml->startElement("status");
      {
        std::string fileCwl(finalPath);
        std::string fileZo(finalPath);
        fileCwl.append(".yaml");
        finalPath.append(".zcfg");
        fileZo.append(".zo");

        std::cerr << fileCwl << "\n";
        std::cerr << fileZo << "\n";

        if (fileExist(finalPath.data()) && removeFile(finalPath.c_str())) {
          xml->writeAttribute("err", "4");
          xml->writeAttribute("message", "Can't remove the service");
          xml->writeContent("not removed");
        } else {
          removeFile(fileCwl.c_str());
          removeFile(fileZo.c_str());
          xml->writeAttribute("err", "0");
          xml->writeAttribute("message", "0");
          xml->writeContent("removed");
        }
      }
      xml->endElement();
    }
    xml->endElement();
  }
  xml->endElement();

  xml->endElement();
  xml->endDocument();

  xml->getBuffer(strBuffer);

  setMapInMaps(outputs, "unDeployResult", "value", strBuffer.str().c_str());
  setStatus(conf, "done", "");
  updateStatus(conf, 100, "done");
  return SERVICE_SUCCEEDED;
}

class User{
  std::string username_{"anonymous"};
  std::string basePath_{""};

  public:
   
    User()=default;
    User(std::string username):username_(std::move(username)){
    }
    void setUsername(std::string username){
      username_=std::move(username);
    }
    void setBasePath(std::string basePath){
      basePath_=std::move(basePath);
      if (*basePath_.rbegin() != '/') {
        basePath_.append("/");
      }
      fprintf(stderr,"setBasePath: %s\n",basePath_.c_str());
    }
    std::string getPath(){
      return  basePath_+username_ + "/";
    }
    std::string prepareUserWorkspace(){
     
      auto p=getPath();
      auto co=std::make_unique<char[]>(p.size()+1);
      memset(co.get(), '\0',p.size()+1);
      memcpy(co.get(), p.c_str(), p.size());
      
      if (mkpath(co.get(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0){
        return "Can't create the user workspace: " + getPath();
      }

      return std::string();
    }
};


int job(maps *&conf, maps *&inputs, maps *&outputs, Operation operation) {

  std::map<std::string, std::string> confEoepca;
  getT2ConfigurationFromZooMapConfig(conf, "eoepca", confEoepca);
  std::string buildPath=confEoepca["buildPath"];

  std::map<std::string, std::string> confMain;
  getT2ConfigurationFromZooMapConfig(conf, "main", confMain);

  if (buildPath.empty()) {
    std::string message{"zoo buildPath empty()"};
    setStatus(conf, "failed", message.c_str());
    updateStatus(conf, 100, message.c_str());
    return setZooError(conf, message, "NoApplicableCode");
  }

  if (confMain["servicePath"].empty()) {
    std::string message{"zoo servicePath empty()"};
    setStatus(conf, "failed", message.c_str());
    updateStatus(conf, 100, message.c_str());
    return setZooError(conf, message, "NoApplicableCode");
  }

  if (confEoepca["userworkspace"].empty()) {
    std::string message{"zoo userworkspace empty()"};
    setStatus(conf, "failed", message.c_str());
    updateStatus(conf, 100, message.c_str());
    return setZooError(conf, message, "NoApplicableCode");
  }

  if (*confEoepca["userworkspace"].rbegin() != '/') {
    confEoepca["userworkspace"].append("/");
  }

  std::map<std::string, std::string> userEoepca;
  getT2ConfigurationFromZooMapConfig(conf, "eoepcaUser", userEoepca);

  auto user = std::make_unique<User>("anonymous");
  if(!confEoepca["defaultUser"].empty()){
   user->setUsername(confEoepca["defaultUser"]); 
  }
  user->setBasePath(confEoepca["userworkspace"]);

  if (!userEoepca["user"].empty()){
    user->setUsername(userEoepca["user"]);
  }

  for(auto& [k,q]:userEoepca){
    fprintf(stderr, "-----> %s,%s  \n",k.c_str(),q.c_str());
  }

  if (userEoepca["grant"].empty()) {
    std::string message{"Grants for this user are not  defined"};
    setStatus(conf, "failed", message.c_str());
    updateStatus(conf, 100, message.c_str());
    return setZooError(conf, message, "NoApplicableCode");
  }else {

    auto grant=userEoepca["grant"];
    if (grant.size()!=3){
      std::string message{"Grants format error: "};
      message.append(grant);
      setStatus(conf, "failed", message.c_str());
      updateStatus(conf, 100, message.c_str());
      return setZooError(conf, message, "NoApplicableCode");
    }

    std::cerr << "user: "<< userEoepca["user"] << " grants: "  << grant << "\n";

    if(grant.at(2) == '-'){
      std::string message{"The user "};
      message.append(userEoepca["user"]).append(" does not have permissions to perform this operation");
      setStatus(conf, "failed", message.c_str());
      updateStatus(conf, 100, message.c_str());
      return setZooError(conf, message, "NoApplicableCode");
    }

  }

  auto resU=user->prepareUserWorkspace();
  if (!resU.empty()){
    std::string message{resU};
    setStatus(conf, "failed", message.c_str());
    updateStatus(conf, 100, message.c_str());
    return setZooError(conf, message, "NoApplicableCode");
  }


  std::map<std::string, std::string> confMetadata;
  getT2ConfigurationFromZooMapConfig(conf, "metadata", confMetadata);

  map *applicationPackageZooMap =
      getMapFromMaps(inputs, "applicationPackage", "value");

  if (!applicationPackageZooMap) {
    return setZooError(conf, "applicationPackage empty()", "NoApplicableCode");
  }

  std::string owsOri(applicationPackageZooMap->value);

  try {

    if (operation == Operation::UNDEPLOY) {
      auto found = owsOri.find("://");
      if (found == std::string::npos) {
        if (owsOri == "eoepcaadesundeployprocess" || owsOri == "GetStatus" ||
            owsOri == "eoepcaadesdeployprocess") {
          std::string err("The service ");
          err.append(owsOri).append(" cannot be removed");
          throw std::runtime_error(err);
        }

        std::string finalPath = confMain["servicePath"];
        finalPath=user->getPath();

        finalPath.append(owsOri);
        fprintf(stderr,"finalPath=%s, getPath=%s",finalPath.c_str(),user->getPath().c_str() );

        return simpleRemove(finalPath, owsOri, conf, inputs, outputs);
      }
    }

    std::string bufferOWSFile;
    auto found = owsOri.find("://");
    if (found == std::string::npos) {
      bufferOWSFile = owsOri;
    } else {
      auto ret = getFromWeb(bufferOWSFile, owsOri.c_str());
      if (ret != 200) {
        std::string err{"Can't download the file: "};
        err.append(applicationPackageZooMap->value);
        throw std::runtime_error(err);
      }
    }

    auto libPath = confEoepca["owsparser"];
    auto lib = std::make_unique<EOEPCA::EOEPCAows>(libPath);
    if (!lib->IsValid()) {
      std::string err{"Can't load the file: "};
      err.append(libPath);
      throw std::runtime_error(err);
    }

    std::list<std::pair<std::string, std::string>> metadata;
    metadata.emplace_back("owsOrigin", std::string(owsOri));

    std::unique_ptr<EOEPCA::OWS::OWSContext,
                    std::function<void(EOEPCA::OWS::OWSContext *)>>
        ptrContext(
            lib->parseFromMemory(bufferOWSFile.c_str(), bufferOWSFile.size()),
            lib->releaseParameter);

    if (ptrContext) {

      auto converter = std::make_unique<ZOO::ZooConverter>();

      std::list<std::string> uniqueTags{};
      uniqueTags.emplace_back(owsOri);

      std::list<std::pair<std::string, std::string>> metadata;
      if (!confMetadata.empty()) {
        for (auto &[k, v] : confMetadata)
          metadata.emplace_back(k, v);
      }
      metadata.emplace_back("owsOri", owsOri);

      auto out = converter->convert(uniqueTags, ptrContext.get(), metadata);

      std::stringbuf strBuffer;
      auto xml = std::make_unique<XmlWriteMemoryWrapper>();
      xml->startDocument("", "", "");
      xml->startElement("result");
      xml->writeAttribute("applicationPackageFile", owsOri);

      std::string finalPath, cwlRef, zooRef;
      for (auto &single : out) {
        xml->startElement("operations");
        {
          xml->writeAttribute("packageId", single->getPackageId());

          xml->startElement("operation");
          {
            for (auto &zoo : single->getZoos()) {
              xml->writeAttribute("processId", zoo->getProcessDescriptionId());
              xml->writeAttribute("processVersion", zoo->getProcessVersion());
              xml->writeAttribute("version", zoo->getProcessVersion());
              xml->writeAttribute("type", operation == Operation::DEPLOY
                                              ? "deploy"
                                              : "undeploy");

              finalPath = confMain["servicePath"];
              fprintf(stderr,"finalPath=%s, getPath=%s",finalPath.c_str(),user->getPath().c_str() );
              finalPath=user->getPath();

              finalPath.append(zoo->getIdentifier());
              cwlRef = finalPath;
              zooRef = finalPath;

              xml->startElement("wpsId");
              { xml->writeContent(zoo->getIdentifier()); }
              xml->endElement();

              xml->startElement("title");
              { xml->writeContent(zoo->getTitle()); }
              xml->endElement();

              xml->startElement("abstract");
              { xml->writeContent(zoo->getAbstract()); }
              xml->endElement();

              switch (operation) {
              case Operation::DEPLOY: {
                finalPath.append(".zcfg");
                cwlRef.append(".yaml");
                zooRef.append(".zo");

                xml->startElement("status");
                if (!fileExist(zooRef.c_str())) {
                  std::string COMPILE =
                      ("make -C " + buildPath +  " USERPATH=\"" +user->getPath() +"\" COMPILE=\"" +
                       zoo->getIdentifier() + "\"  1>&2  ");
                  
                  std::cerr << "***" << COMPILE << "\n";
                  int COMPILERES = system((char *)COMPILE.c_str());
                  sleep(2);
                }

                if (!fileExist(zooRef.c_str())) {
                  removeFile(finalPath.c_str());
                  removeFile(cwlRef.c_str());
                  removeFile(zooRef.c_str());
                  throw std::runtime_error("Error during the building phase!");
                }

                switch (deploy(finalPath, zoo->getConfigFile())) {
                case DeployResults::NONE: {
                  deploy(cwlRef, zoo->getCwlContent());
                  xml->writeAttribute("err", "0");
                  xml->writeAttribute("mess", "");
                  xml->writeContent("ready");
                  break;
                }
                case DeployResults::CANTWRITE: {
                  xml->writeAttribute("err", "1");
                  xml->writeAttribute("message",
                                      "Can' t write all necessary files");
                  xml->writeContent("not ready");
                  break;
                }
                case DeployResults::EXIST: {
                  xml->writeAttribute("err", "2");
                  xml->writeAttribute("message", "Service already installed");
                  xml->writeContent("not ready");
                  break;
                }
                }
                xml->endElement();
                break;
              }
              case Operation::UNDEPLOY: {
                finalPath.append(".zcfg");
                xml->startElement("status");

                if (fileExist(finalPath.c_str()) &&
                    removeFile(finalPath.c_str())) {
                  xml->writeAttribute("err", "4");
                  xml->writeAttribute("message", "Can't remove the service");
                  xml->writeContent("not removed");
                } else {
                  xml->writeAttribute("err", "0");
                  xml->writeAttribute("message", "0");
                  xml->writeContent("removed");
                }

                xml->endElement();
                break;
              }
              }
            }
          }
          xml->endElement();
        }
        xml->endElement();
      }

      xml->endElement();
      xml->endDocument();

      xml->getBuffer(strBuffer);

      switch (operation) {
      case Operation::DEPLOY: {
        setMapInMaps(outputs, "deployResult", "value", strBuffer.str().c_str());
        break;
      }

      case Operation::UNDEPLOY: {
        setMapInMaps(outputs, "unDeployResult", "value",
                     strBuffer.str().c_str());
        break;
      }
      }

    } else {
      throw std::runtime_error("Error during ows parse!");
    }

  } catch (std::runtime_error &err) {
    std::cerr << err.what();
    setStatus(conf, "failed", err.what());
    updateStatus(conf, 100, err.what() );
    return setZooError(conf, err.what(), "NoApplicableCode");
  } catch (...) {
    std::cerr << "Unexpected server error";
    setStatus(conf, "failed", "Unexpected server error");
    updateStatus(conf, 100, "Unexpected server error");
    return setZooError(conf, "Unexpected server error", "NoApplicableCode");
  }

  setStatus(conf, "done", "");
  // setMapInMaps(outputs, "deployResult", "value", "http://www.me.i22222t");
  return SERVICE_SUCCEEDED;
}
extern "C" {

ZOO_DLL_EXPORT int eoepcaadesdeployprocess(maps *&conf, maps *&inputs,
                                           maps *&outputs){
  dumpMaps(inputs);
  dumpMaps(conf);

  return job(conf, inputs, outputs, Operation::DEPLOY);
}

ZOO_DLL_EXPORT int eoepcaadesundeployprocess(maps *&conf, maps *&inputs,
                                             maps *&outputs) {
  dumpMaps(inputs);
  dumpMaps(conf);
  return job(conf, inputs, outputs, Operation::UNDEPLOY);
}
}
