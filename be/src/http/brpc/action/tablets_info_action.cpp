// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
#include "tablets_info_action.h"

#include <brpc/http_method.h>

#include "olap/storage_engine.h"
#include "olap/tablet_manager.h"

namespace doris {
TabletsInfoHandler::TabletsInfoHandler() : BaseHttpHandler("tablets_info") {}

void TabletsInfoHandler::handle_sync(brpc::Controller* cntl) {
    const std::string* limit = get_param(cntl, "limit");
    std::string tablet_num_to_return;
    if (limit != nullptr) {
        tablet_num_to_return = *limit;
    }
    on_succ_json(cntl, get_tablets_info(cntl, tablet_num_to_return).ToString());
}

bool TabletsInfoHandler::support_method(brpc::HttpMethod method) const {
    return method == brpc::HTTP_METHOD_GET;
}

EasyJson TabletsInfoHandler::get_tablets_info(brpc::Controller* cntl,
                                              const std::string& tablet_num_to_return) {
    int64_t number;
    std::string msg;
    if (tablet_num_to_return == "") {
        number = 1000; // default
        msg = "OK";
    } else if (tablet_num_to_return == "all") {
        number = std::numeric_limits<std::int64_t>::max();
        msg = "OK";
    } else if (std::all_of(tablet_num_to_return.begin(), tablet_num_to_return.end(), ::isdigit)) {
        number = std::atol(tablet_num_to_return.c_str());
        msg = "OK";
    } else {
        number = 0;
        msg = "Parameter Error";
    }
    std::vector<TabletInfo> tablets_info;
    TabletManager* tablet_manager = StorageEngine::instance()->tablet_manager();
    tablet_manager->obtain_specific_quantity_tablets(tablets_info, number);

    EasyJson tablets_info_ej;
    tablets_info_ej["msg"] = msg;
    tablets_info_ej["code"] = 0;
    EasyJson data = tablets_info_ej.Set("data", EasyJson::kObject);
    data["host"] = get_localhost(cntl).c_str();
    EasyJson tablets = data.Set("tablets", EasyJson::kArray);
    for (TabletInfo tablet_info : tablets_info) {
        EasyJson tablet = tablets.PushBack(EasyJson::kObject);
        tablet["tablet_id"] = tablet_info.tablet_id;
        tablet["schema_hash"] = tablet_info.schema_hash;
    }
    tablets_info_ej["count"] = tablets_info.size();
    return tablets_info_ej;
}
} // namespace doris
