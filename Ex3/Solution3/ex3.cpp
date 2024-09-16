
#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "ex3.h"

// Reads the configuration file, create the list of producers and return the number of Co-Editors.
int readConfigurationFile(std::vector<Producer*> &producers, char* path, int &semaphores) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("Failed to open the file.\n");
        exit(1);
    }

    char buffer[100];
    int queueSize, numProducts, producerNumber;
    int coEditorQueueSize = 0;
    int cPR = 0;

    // Read the file line by line
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (strncmp(buffer, "PRODUCER", 8) == 0) {
            producerNumber = atoi(&buffer[9]);
            if (fgets(buffer, sizeof(buffer), file) != NULL) {
                numProducts = atoi(buffer);
            }
            if (fgets(buffer, sizeof(buffer), file) != NULL) {
                sscanf(buffer, "queue size = %d", &queueSize);
                semaphores++;
                cPR++;
                producers.push_back(new Producer(producerNumber, queueSize, numProducts, semaphores));
            }
        } else if (strncmp(buffer, "Co-Editor queue size =", 22) == 0) {
            sscanf(buffer, "Co-Editor queue size = %d", &coEditorQueueSize);
        }
    }

    fclose(file);
    return coEditorQueueSize;
}

// Create strings from the producer
void *runProducer(void *producer) {
    Producer p = (Producer *) producer;
    int flag = 1;

    while (flag) {
        flag = p.createString();
    }
    return NULL;
}

// Get the strings from the producers and transfer them to the co-editors queue.
void *runDispatcher(void *dispatcher) {
    Dispatcher *d = (Dispatcher *) dispatcher;

    int current = -1, finishedProducers[d->_numOfProducers], counter = 0, type;

    while (counter != d->_numOfProducers) {

        usleep(50000);
        current++;

        if (current == d->_numOfProducers) {
            current = 0;
        }

        if (finishedProducers[current]) {
            continue;
        }

        std::string str = d->_producers[current]->_buffer->remove();

        if (str == "empty")
            continue;

        if (str == DONE) {
            finishedProducers[current] = 1;
            counter++;
            continue;
        }

        type = 0;
        if (str.find("NEWS") != std::string::npos)
            type = 1;
        else if (str.find("WEATHER") != std::string::npos)
            type = 2;
        d->_buffers[type]->insert(str);
    }

    d->_buffers[0]->insert(DONE);
    d->_buffers[1]->insert(DONE);
    d->_buffers[2]->insert(DONE);
    return NULL;
}

// Get each string in the compatible category from the dispatcher and transfer it to the buffer.
void *runCoEditor(void* coEditor) {
    CoEditor *c = (CoEditor *) coEditor;
    std::string str;

    while ((str = c->_unBoundedBuffer->remove()) != DONE) {

        usleep(50000);

        if (str == "empty") {
            continue;
        }

        while (c->_boundedBuffer->isFull()) {
            usleep(50000);
        }

        c->_boundedBuffer->insert(str);
    }

    while (c->_boundedBuffer->isFull()) {
        usleep(50000);
    }
    c->_boundedBuffer->insert(DONE);
    return NULL;
}

// Get each string from the co-editors shared queue and print it.
void runScreenManager(BoundedBuffer *coEditorsSharedQueue) {
    int count = 0;

    while (count != NUM_CO_EDITORS) {
        std::string m = coEditorsSharedQueue->remove();

        if (m == "empty")
            continue;

        if (m == DONE) {
            count++;
            continue;
        }
        std::cout << m << std::endl;
    }
}

int main(int argc, char** argv) {

    if (argc != 2) {
        return -1;
    }

    std::vector<Producer*> producers;
    int semaphores = 0, i;

    int bufferSize = readConfigurationFile(producers, argv[1], semaphores);
    int numberOfProducers = semaphores;

    pthread_t producerThread[numberOfProducers];
    pthread_t dispatcherThread;
    pthread_t coEditorThread[NUM_CO_EDITORS];

    BoundedBuffer *coEditorsSharedQueue = new BoundedBuffer(bufferSize, ++semaphores);

    std::vector<UnBoundedBuffer*> dispatcherBuffers;
    for (i = 0; i < NUM_CO_EDITORS; i++) {
        dispatcherBuffers.push_back(new UnBoundedBuffer(++semaphores));
    }

    Dispatcher* dispatcher = new Dispatcher(producers, dispatcherBuffers, numberOfProducers);
    std::vector<CoEditor*> coEditors;
    for (i = 0; i < NUM_CO_EDITORS; i++) {
        coEditors.push_back(new CoEditor(dispatcherBuffers[i], coEditorsSharedQueue));
    }

    if (pthread_create(&dispatcherThread, NULL, runDispatcher, (void *) dispatcher) != 0) {
        exit(1);
    }
    for (i = 0; i < numberOfProducers; i++) {
        if (pthread_create(&producerThread[i], NULL, runProducer, (void *) producers[i]) != 0) {
            exit(1);
        }
    }
    for (i = 0; i < NUM_CO_EDITORS; i++) {
        if (pthread_create(&coEditorThread[i], NULL, runCoEditor, (void *) coEditors[i]) != 0) {
            exit(1);
        }
    }
    runScreenManager(coEditorsSharedQueue);
    std::cout << "DONE";
    return 0;
}
