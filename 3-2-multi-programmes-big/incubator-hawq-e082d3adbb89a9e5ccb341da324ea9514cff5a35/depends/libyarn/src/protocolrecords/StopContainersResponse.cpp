/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "StopContainersResponse.h"

namespace libyarn {

StopContainersResponse::StopContainersResponse() {
	responseProto = StopContainersResponseProto::default_instance();
}

StopContainersResponse::StopContainersResponse(
		const StopContainersResponseProto &proto) {
	responseProto = proto;
}

StopContainersResponse::~StopContainersResponse() {
}

StopContainersResponseProto& StopContainersResponse::getProto() {
	return responseProto;
}

void StopContainersResponse::setSucceededRequests(list<ContainerId> requests) {
	for (list<ContainerId>::iterator it = requests.begin();
			it != requests.end(); it++) {
		ContainerIdProto *proto = responseProto.add_succeeded_requests();
		proto->CopyFrom((*it).getProto());
	}
}

list<ContainerId> StopContainersResponse::getSucceededRequests() {
	list<ContainerId> cids;
	int size = responseProto.succeeded_requests_size();
	for (int i = 0; i < size; i++) {
		ContainerIdProto proto = responseProto.succeeded_requests(i);
		ContainerId cid(proto);
		cids.push_back(cid);
	}
	return cids;
}

void StopContainersResponse::setFailedRequests(
		list<ContainerExceptionMap> & requests) {
	for (list<ContainerExceptionMap>::iterator it = requests.begin();
			it != requests.end(); it++) {
		ContainerExceptionMapProto *proto = responseProto.add_failed_requests();
		proto->CopyFrom((*it).getProto());
	}
}

list<ContainerExceptionMap> StopContainersResponse::getFailedRequests() {
	list<ContainerExceptionMap> ces;
	int size = responseProto.failed_requests_size();
	for (int i = 0; i < size; i++) {
		ContainerExceptionMapProto proto = responseProto.failed_requests(i);
		ces.push_back(ContainerExceptionMap(proto));
	}
	return ces;
}

} /* namespace libyarn */
